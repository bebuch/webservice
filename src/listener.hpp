//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__listener__hpp_INCLUDED_
#define _webservice__listener__hpp_INCLUDED_

#include <webservice/http_request_handler.hpp>

#include "http_session.hpp"

#include <boost/asio/defer.hpp>


namespace webservice{


	/// \brief Accepts incoming connections and launches the sessions
	class listener{
	public:
		listener(
			std::unique_ptr< http_request_handler > handler,
			std::unique_ptr< ws_handler_base > service,
			std::unique_ptr< error_handler > error_handler,
			boost::asio::ip::tcp::endpoint endpoint,
			std::uint8_t thread_count,
			boost::optional< std::chrono::milliseconds > websocket_ping_time,
			std::size_t max_read_message_size
		)
			: handler_(std::move(handler))
			, service_(std::move(service))
			, error_handler_(std::move(error_handler))
			, websocket_ping_time_(websocket_ping_time)
			, max_read_message_size_(max_read_message_size)
			, ioc_{thread_count}
			, acceptor_(ioc_)
			, socket_(ioc_)
		{
			// Open the acceptor
			acceptor_.open(endpoint.protocol());

			// Bind to the server address
			acceptor_.bind(endpoint);

			// Start listening for connections
			acceptor_.listen(boost::asio::socket_base::max_listen_connections);

			// Start accepting incoming connections
			BOOST_ASSERT(acceptor_.is_open());
			do_accept();
		}

		void do_accept(){
			acceptor_.async_accept(
				socket_,
				[this](boost::system::error_code ec){
					on_accept(ec);
				});
		}

		void on_accept(boost::system::error_code ec){
			if(ec){
				try{
					error_handler_->on_error(ec);
				}catch(...){
					on_exception(std::current_exception());
				}
			}else{
				// Create the http_session and run it
				auto session = std::make_shared< http_session >(
					std::move(socket_), *handler_, service_,
					websocket_ping_time_, max_read_message_size_);

				session->run();
			}

			// Accept another connection
			do_accept();
		}

		void run(){
			ioc_.run();
		}

		void stop()noexcept{
			ioc_.stop();
		}


		/// \brief Execute a function async via server threads
		void async(std::function< void() >&& fn){
			boost::asio::defer(ioc_, [this, fn = std::move(fn)]()noexcept{
					try{
						fn();
					}catch(...){
						on_exception(std::current_exception());
					}
				});
		}


		/// \brief Called when an exception in the server occurred
		void on_exception(std::exception_ptr error)noexcept{
			error_handler_->on_exception(error);
		}


		/// \brief Run one task in server threads
		void run_one()noexcept{
			try{
				ioc_.run_one();
			}catch(...){
				on_exception(std::current_exception());
			}
		}


	private:
		std::unique_ptr< http_request_handler > handler_;
		std::unique_ptr< ws_handler_base > service_;
		std::unique_ptr< error_handler > error_handler_;
		boost::optional< std::chrono::milliseconds > const websocket_ping_time_;
		std::size_t const max_read_message_size_;

		/// \brief The io_context is required for all I/O
		boost::asio::io_context ioc_;

		boost::asio::ip::tcp::acceptor acceptor_;
		boost::asio::ip::tcp::socket socket_;
	};


}


#endif
