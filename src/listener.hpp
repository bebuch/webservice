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


namespace webservice{


	/// \brief Accepts incoming connections and launches the sessions
	class listener{
	public:
		listener(
			server_impl& server,
			boost::asio::ip::tcp::endpoint endpoint,
			boost::asio::io_context& ioc
		)
			: server_(std::move(server))
			, acceptor_(ioc)
			, socket_(ioc)
		{
			// Open the acceptor
			acceptor_.open(endpoint.protocol());

			// Allow port reuse
			using acceptor = boost::asio::ip::tcp::acceptor;
			acceptor_.set_option(acceptor::reuse_address(true));

			// Bind to the server address
			acceptor_.bind(endpoint);

			// Start listening for connections
			acceptor_.listen(boost::asio::socket_base::max_listen_connections);

			// Start accepting incoming connections
			do_accept();
		}


		void do_accept(){
			acceptor_.async_accept(
				socket_,
				[this](boost::system::error_code ec){
					if(ec == boost::asio::error::operation_aborted){
						return;
					}

					if(ec){
						try{
							server_.error().on_error(ec);
						}catch(...){
							server_.error().on_exception(
								std::current_exception());
						}
					}else{
						// Create the http_session and run it
						server_.http().emplace(std::move(socket_))->run();
					}

					// Accept another connection
					do_accept();
				});
		}

		void shutdown()noexcept{
			// Don't accept new sessions
			acceptor_.close();
		}


	private:
		boost::asio::ip::tcp::acceptor acceptor_;
		boost::asio::ip::tcp::socket socket_;
	};


}


#endif
