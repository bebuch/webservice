//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <webservice/ws_handler_interface.hpp>


namespace webservice{



	void ws_handler_interface::set_server(class server& server)noexcept{
		server_ = &server;
		on_server();
	}

	void ws_handler_interface::make(
		boost::asio::ip::tcp::socket&& socket,
		http_request&& req
	){
		on_make(std::move(socket), std::move(req));
	}

	void ws_handler_interface::shutdown()noexcept{
		on_shutdown();
	}


	class server* ws_handler_interface::server()noexcept{
		return server_;
	}


	void ws_handler_interface::on_exception(
		std::exception_ptr /*error*/)noexcept{}


}
