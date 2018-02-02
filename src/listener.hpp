//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webserver
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webserver__listener__hpp_INCLUDED_
#define _webserver__listener__hpp_INCLUDED_

#include <webserver/request_handler.hpp>


namespace webserver{


	/// \brief Accepts incoming connections and launches the sessions
	class listener{
	public:
		listener(
			request_handler& handler,
			boost::asio::io_context& ioc,
			boost::asio::ip::tcp::endpoint endpoint
		)
			: handler_(handler)
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
			if(ec == boost::asio::error::invalid_argument){
				log_fail(ec, "listener accept");
				return;
			}else if(ec){
				log_fail(ec, "listener accept");
			}else{
				// Create the http_session and run it
				handler_(std::move(socket_));
			}

			// Accept another connection
			do_accept();
		}

	private:
		request_handler& handler_;
		boost::asio::ip::tcp::acceptor acceptor_;
		boost::asio::ip::tcp::socket socket_;
	};


}


#endif
