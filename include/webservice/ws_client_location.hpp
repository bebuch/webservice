//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__ws_client_location__hpp_INCLUDED_
#define _webservice__ws_client_location__hpp_INCLUDED_

#include <boost/beast/core/string.hpp>


namespace webservice{


	enum class ws_client_location{
		read,
		write,
		close,
		timer,
		ping
	};

	constexpr boost::beast::string_view
	to_string_view(ws_client_location location){
		switch(location){
			case ws_client_location::read: return "read";
			case ws_client_location::write: return "write";
			case ws_client_location::close: return "close";
			case ws_client_location::timer: return "timer";
			case ws_client_location::ping: return "ping";
			default: return "invalid location";
		}
	}


}


#endif
