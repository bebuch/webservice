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


namespace webservice{


	/// \brief Accepts incoming connections and launches the sessions
	class listener{
	public:
		listener(
			std::unique_ptr< http_request_handler > handler,
			std::unique_ptr< ws_service_base > service,
			std::unique_ptr< error_handler > error_handler,
			boost::asio::io_context& ioc,
			boost::asio::ip::tcp::endpoint endpoint,
			boost::optional< std::chrono::milliseconds > websocket_ping_time,
			std::size_t max_read_message_size
		)
			: handler_(std::move(handler))
			, service_(std::move(service))
			, error_handler_(std::move(error_handler))
			, websocket_ping_time_(websocket_ping_time)
			, max_read_message_size_(max_read_message_size)
			, acceptor_(ioc)
			, socket_(ioc)
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
					error_handler_->on_exception(std::current_exception());
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

		/// \brief Called when an exception in the server occurred
		virtual void on_exception(std::exception_ptr error)noexcept{
			error_handler_->on_exception(error);
		}

	private:
		std::unique_ptr< http_request_handler > handler_;
		std::unique_ptr< ws_service_base > service_;
		std::unique_ptr< error_handler > error_handler_;
		boost::optional< std::chrono::milliseconds > const websocket_ping_time_;
		std::size_t const max_read_message_size_;
		boost::asio::ip::tcp::acceptor acceptor_;
		boost::asio::ip::tcp::socket socket_;
	};


}


#endif
