//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__ws_service_location__hpp_INCLUDED_
#define _webservice__ws_service_location__hpp_INCLUDED_

#include <boost/beast/core/string.hpp>


namespace webservice{


	enum class ws_service_location{
		accept,
		read,
		write,
		timer,
		ping
	};

	constexpr boost::beast::string_view
	to_string_view(ws_service_location location){
		switch(location){
			case ws_service_location::accept: return "accept";
			case ws_service_location::read: return "read";
			case ws_service_location::write: return "write";
			case ws_service_location::timer: return "timer";
			case ws_service_location::ping: return "ping";
			default: return "invalid location";
		}
	}


}


#endif
