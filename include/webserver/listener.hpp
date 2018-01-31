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

#include "http_session.hpp"


namespace webserver{


	// Accepts incoming connections and launches the sessions
	class listener: public std::enable_shared_from_this< listener >{
	public:
		listener(
			boost::asio::io_context& ioc,
			boost::asio::ip::tcp::endpoint endpoint,
			std::string const& doc_root
		)
			: acceptor_(ioc)
			, socket_(ioc)
			, doc_root_(doc_root)
		{
			boost::system::error_code ec;

			// Open the acceptor
			acceptor_.open(endpoint.protocol(), ec);
			if(ec){
				fail(ec, "open");
				return;
			}

			// Bind to the server address
			acceptor_.bind(endpoint, ec);
			if(ec){
				fail(ec, "bind");
				return;
			}

			// Start listening for connections
			acceptor_.listen(
				boost::asio::socket_base::max_listen_connections, ec);
			if(ec){
				fail(ec, "listen");
				return;
			}
		}

		// Start accepting incoming connections
		void run(){
			if(!acceptor_.is_open()){
				return;
			}
			do_accept();
		}

		void do_accept(){
			acceptor_.async_accept(
				socket_,
				[this_ = shared_from_this()](boost::system::error_code ec){
					this_->on_accept(ec);
				});
		}

		void on_accept(boost::system::error_code ec){
			if(ec){
				fail(ec, "accept");
			}else{
				// Create the http_session and run it
				std::make_shared< http_session >(
					std::move(socket_),
					doc_root_)->run();
			}

			// Accept another connection
			do_accept();
		}

	private:
		boost::asio::ip::tcp::acceptor acceptor_;
		boost::asio::ip::tcp::socket socket_;
		std::string const& doc_root_;
	};


}


#endif
