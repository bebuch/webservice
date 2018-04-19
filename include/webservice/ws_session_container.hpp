//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__ws_session_container__hpp_INCLUDED_
#define _webservice__ws_session_container__hpp_INCLUDED_

#include "ws_identifier.hpp"
#include "async_lock.hpp"

#include <boost/beast/websocket.hpp>

#include <boost/asio/strand.hpp>

#include <map>
#include <tuple>


namespace webservice{


	using ws_stream
		= boost::beast::websocket::stream< boost::asio::ip::tcp::socket >;

	using ws_strand
		= boost::asio::strand< boost::asio::io_context::executor_type >;

	using http_request
		= boost::beast::http::request< boost::beast::http::string_body >;


	class ws_server_session;

	template < typename Value >
	class ws_service_base;


	template < typename Value >
	class ws_session_container{

	public:





	private:
	};


}


#endif
