//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__http_request_location__hpp_INCLUDED_
#define _webservice__http_request_location__hpp_INCLUDED_

#include <boost/beast/core/string.hpp>


namespace webservice{


	enum class http_request_location{
		read,
		write,
		timer
	};

	constexpr boost::beast::string_view
	to_string_view(http_request_location location){
		switch(location){
			case http_request_location::read: return "read";
			case http_request_location::write: return "write";
			case http_request_location::timer: return "timer";
			default: return "invalid location";
		}
	}


}


#endif
