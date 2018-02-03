//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webserver
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webserver__http_session__hpp_INCLUDED_
#define _webserver__http_session__hpp_INCLUDED_

#include <webserver/http_session.hpp>

#include <webserver/fail.hpp>
#include <webserver/handle_request.hpp>
#include <webserver/websocket_session.hpp>

#include <boost/beast/websocket.hpp>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/bind_executor.hpp>


namespace webserver{


	/// \brief Handles an HTTP server connection
	class http_session: public std::enable_shared_from_this< http_session >{
	public:
		/// \brief Take ownership of the socket
		explicit http_session(
			boost::asio::ip::tcp::socket&& socket,
			std::string const& doc_root
		)
			: socket_(std::move(socket))
			, strand_(socket_.get_executor())
			, timer_(socket_.get_executor().context(),
				(std::chrono::steady_clock::time_point::max)())
			, doc_root_(doc_root)
			, queue_(*this) {}

		/// \brief Start the asynchronous operation
		void run(){
			// Run the timer. The timer is operated
			// continuously, this simplifies the code.
			on_timer({});

			do_read();
		}

		void do_read(){
			// Set the timer
			timer_.expires_after(std::chrono::seconds(15));

			// Read a request
			boost::beast::http::async_read(socket_, buffer_, req_,
				boost::asio::bind_executor(
					strand_,
					[this_ = self_.shared_from_this()](
						boost::system::error_code ec,
						std::size_t /*bytes_transferred*/
					){
						this_->on_read(ec);
					}));
		}

		// Called when the timer expires.
		void on_timer(boost::system::error_code ec){
			if(ec && ec != boost::asio::error::operation_aborted){
				return log_fail(ec, "timer");
			}

			// Verify that the timer really expired since the deadline may have
			// moved.
			if(timer_.expiry() <= std::chrono::steady_clock::now()){
				// Closing the socket cancels all outstanding operations. They
				// will complete with boost::asio::error::operation_aborted
				socket_.shutdown(
					boost::asio::ip::tcp::socket::shutdown_both, ec);
				socket_.close(ec);
				return;
			}

			// Wait on the timer
			timer_.async_wait(
				boost::asio::bind_executor(
					strand_,
					[this_ = shared_from_this()](boost::system::error_code ec){
						this_->on_timer(ec);
					}));
		}

		void on_read(boost::system::error_code ec){
			// Happens when the timer closes the socket
			if(ec == boost::asio::error::operation_aborted){
				return;
			}

			// This means they closed the connection
			if(ec == boost::beast::http::error::end_of_stream){
				return do_close();
			}

			if(ec){
				return log_fail(ec, "read");
			}

			// See if it is a WebSocket Upgrade
			if(boost::beast::websocket::is_upgrade(req_)){
				// Create a WebSocket websocket_session by transferring the
				// socket
				std::make_shared< websocket_session >(
					std::move(socket_))->do_accept(std::move(req_));
			}else{
				// Send the response
				handle_request(doc_root_, std::move(req_), queue_);

				// If we aren't at the queue limit, try to pipeline another request
				if(!queue_.is_full()){
					do_read();
				}
			}
		}

		void on_write(boost::system::error_code ec, bool close){
			// Happens when the timer closes the socket
			if(ec == boost::asio::error::operation_aborted){
				return;
			}

			if(ec){
				return log_fail(ec, "write");
			}

			if(close){
				// This means we should close the connection, usually because
				// the response indicated the "Connection: close" semantic.
				return do_close();
			}

			// Inform the queue that a write completed
			if(queue_.on_write()){
				// Read another request
				do_read();
			}
		}

		void do_close(){
			// Send a TCP shutdown
			boost::system::error_code ec;
			socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);

			// At this point the connection is closed gracefully
		}

	private:
		// This queue is used for HTTP pipelining.
		class queue{
		public:
			explicit queue(http_session& self)
				: self_(self)
			{
				static_assert(limit > 0, "queue limit must be positive");
				items_.reserve(limit);
			}

			// Returns `true` if we have reached the queue limit
			bool is_full()const{
				return items_.size() >= limit;
			}

			// Called when a message finishes sending
			// Returns `true` if the caller should initiate a read
			bool on_write(){
				BOOST_ASSERT(!items_.empty());
				auto const was_full = is_full();
				items_.erase(items_.begin());
				if(!items_.empty()){
					(*items_.front())();
				}
				return was_full;
			}

			// Called by the HTTP handler to send a response.
			template < typename Body, typename Fields >
			void operator()(
				boost::beast::http::respons< Body, Fields >&& msg
			){
				// This holds a work item
				class work_impl: public work{
				public:
					work_impl(
						http_session& self,
						boost::beast::http::respons< Body, Fields >&& msg
					)
						: self_(self)
						, msg_(std::move(msg)) {}

					void operator()(){
						boost::beast::http::async_write(
							self_.socket_,
							msg_,
							boost::asio::bind_executor(
								self_.strand_,
								[
									this_ = self_.shared_from_this(),
									need_eof = msg_.need_eof()
								](
									boost::system::error_code ec,
									std::size_t /*bytes_transferred*/
								){
									this_->on_write(ec, need_eof);
								}));
					}

				private:
					http_session& self_;
					boost::beast::http::respons< Body, Fields > msg_;
				};

				// Allocate and store the work
				items_.push_back(
					std::make_unique< work_impl >(self_, std::move(msg)));

				// If there was no previous work, start this one
				if(items_.size() == 1){
					(*items_.front())();
				}
			}

		private:
			// Maximum number of responses we will queue
			static constexpr std::size_t limit = 64;

			// The type-erased, saved work item
			struct work{
				virtual ~work() = default;
				virtual void operator()() = 0;
			};

			http_session& self_;
			std::vector< std::unique_ptr< work > > items_;
		};


		boost::asio::ip::tcp::socket socket_;
		boost::asio::strand< boost::asio::io_context::executor_type > strand_;
		boost::asio::steady_timer timer_;
		boost::beast::flat_buffer buffer_;
		std::string const& doc_root_;
		boost::beast::http::request< boost::beast::http::string_body > req_;
		queue queue_;
	};


}


#endif
