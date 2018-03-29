//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__http_session__hpp_INCLUDED_
#define _webservice__http_session__hpp_INCLUDED_

#include "server_impl.hpp"

#include <webservice/async_lock.hpp>
#include <webservice/http_response.hpp>
#include <webservice/http_request_handler.hpp>
#include <webservice/ws_handler_base.hpp>
#include <webservice/error_handler.hpp>

#include <boost/circular_buffer.hpp>

#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/websocket.hpp>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/bind_executor.hpp>


namespace webservice{


	/// \brief Handles an HTTP server session
	class http_session{
	public:
		/// \brief Take ownership of the socket and start reading
		explicit http_session(
			boost::asio::ip::tcp::socket&& socket,
			server_impl& server
		)
			: server_(server)
			, socket_(std::move(socket))
			, strand_(socket_.get_executor())
			, timer_(socket_.get_executor().context(),
				std::chrono::steady_clock::time_point::max())
			{}

		~http_session(){
			// Send an async shutdown to any session
			boost::asio::post(boost::asio::bind_executor(
				strand_,
				[this, lock = async_lock(async_calls_)]{
					boost::system::error_code ec;
					timer_.cancel(ec);
					if(socket_.is_open()){
						using socket = boost::asio::ip::tcp::socket;
						socket_.shutdown(socket::shutdown_both, ec);
						socket_.close(ec);
					}
				}));

			// As long as async calls are pending
			while(async_calls_ > 0){
				// Request the server to run a handler async
				if(server_.poll_one() == 0){
					// If no handler was waiting, the pending one must
					// currently run in another thread
					std::this_thread::yield();
				}
			}
		}

		// Called when the timer expires.
		void do_timer(){
			timer_.async_wait(boost::asio::bind_executor(
				strand_,
				[this, lock = async_lock(async_calls_)](
					boost::system::error_code ec
				){
					if(ec == boost::asio::error::operation_aborted){
						return;
					}

					if(ec){
						try{
							server_.http().on_error(
								http_request_location::timer, ec);
						}catch(...){
							server_.http().on_exception(
								std::current_exception());
						}
					}else{
						// Closing the socket cancels all outstanding operations.
						// They will complete with operation_aborted
						using socket = boost::asio::ip::tcp::socket;
						socket_.shutdown(socket::shutdown_both, ec);
						socket_.close(ec);
					}
					async_erase();
				}));
		}

		void run(){
			do_timer();

			// Start the asynchronous operation
			do_read();
		}

		void do_read(){
			// Set the timer
			if(timer_.expires_after(server_.http().timeout()) == 0){
				// if the timer could not be cancelled it was already
				// expired and the session was closed by the timer
				return;
			}else{
				// If the timer was cancelled, restart it
				do_timer();
			}

			// Read a request
			boost::beast::http::async_read(socket_, buffer_, req_,
				boost::asio::bind_executor(
					strand_,
					[this, lock = async_lock(async_calls_)](
						boost::system::error_code ec,
						std::size_t /*bytes_transferred*/
					){
						// Happens when the timer closes the socket
						if(ec == boost::asio::error::operation_aborted){
							return;
						}

						// This means endpoint closed the session
						if(ec == boost::beast::http::error::end_of_stream){
							do_close();
							return;
						}

						if(ec){
							try{
								server_.http().on_error(
									http_request_location::read, ec);
							}catch(...){
								server_.http().on_exception(
									std::current_exception());
							}
							async_erase();
							return;
						}

						// See if it is a WebSocket Upgrade
						if(
							server_.has_ws() &&
							boost::beast::websocket::is_upgrade(req_)
						){
							server_.ws().emplace(
								std::move(socket_), std::move(req_));
						}else{
							// Send the response
							server_.http()(std::move(req_), http_response{
									this,
									async_calls_,
									&http_session::response,
									socket_,
									strand_
								});

							// If we aren't at the queue limit, try to pipeline
							// another request
							if(!queue_.is_full()){
								do_read();
							}
						}
					}));
		}

		void on_write(boost::system::error_code ec, bool close){
			// Happens when the timer closes the socket
			if(ec == boost::asio::error::operation_aborted){
				return;
			}

			if(ec){
				try{
					server_.http().on_error(http_request_location::write, ec);
				}catch(...){
					server_.http().on_exception(std::current_exception());
				}
				async_erase();
				return;
			}

			if(close){
				// This means we should close the session, usually because
				// the response indicated the "Connection: close" semantic.
				do_close();
				return;
			}

			// Inform the queue that a write completed
			if(queue_.on_write()){
				// Read another request
				do_read();
			}
		}

		/// \brief Send a TCP shutdown
		void do_close(){
			boost::system::error_code ec;
			using socket = boost::asio::ip::tcp::socket;
			socket_.shutdown(socket::shutdown_send, ec);
			async_erase();
		}

		/// \brief Called by the HTTP handler to send a response.
		void response(std::unique_ptr< http_session_work >&& work){
			queue_.response(std::move(work));
		}

		/// \brief Set the function that is called on async_erase
		void set_erase_fn(sessions_erase_fn< http_session >&& erase_fn)noexcept{
			erase_fn_ = std::move(erase_fn);
		}

		/// \brief Send a request to erase this session from the list
		///
		/// The request is sended only once, any call after the fist will be
		/// ignored.
		void async_erase(){
			std::call_once(erase_flag_, [this]{
					boost::asio::post([this]{
							erase_fn_();
						});
				});
		}


	private:
		// This queue is used for HTTP pipelining.
		class queue{
			/// \brief Maximum number of responses we will queue
			static constexpr std::size_t limit = 64;

		public:
			static_assert(limit > 0, "queue limit must be positive");

			explicit queue():
				items_(limit) {}

			// Returns `true` if we have reached the queue limit
			bool is_full()const{
				return items_.size() >= limit;
			}

			// Called when a message finishes sending
			// Returns `true` if the caller should initiate a read
			bool on_write(){
				BOOST_ASSERT(!items_.empty());
				auto const was_full = is_full();
				items_.pop_front();
				if(!items_.empty()){
					(*items_.front())();
				}
				return was_full;
			}

			// Called by the HTTP handler to send a response.
			void response(std::unique_ptr< http_session_work >&& work){
				// Allocate and store the work
				items_.push_back(std::move(work));

				// If there was no previous work, start this one
				if(items_.size() == 1){
					(*items_.front())();
				}
			}


		private:
			boost::circular_buffer< std::unique_ptr< http_session_work > >
				items_;
		};


		server_impl& server_;

		boost::asio::ip::tcp::socket socket_;
		boost::asio::strand< boost::asio::io_context::executor_type > strand_;
		boost::asio::steady_timer timer_;
		boost::beast::flat_buffer buffer_;

		http_request req_;
		queue queue_;

		sessions_erase_fn< http_session > erase_fn_;

		std::once_flag erase_flag_;
		std::atomic< std::size_t > async_calls_{0};
	};


	void http_session_on_write::operator()(
		boost::system::error_code ec,
		std::size_t /*bytes_transferred*/
	)const{
		self_->on_write(ec, need_eof_);
	}


}


#endif
