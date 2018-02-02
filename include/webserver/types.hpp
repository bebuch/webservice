//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webserver
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webserver__types__hpp_INCLUDED_
#define _webserver__types__hpp_INCLUDED_


namespace webserver{


	/// \brief Server request type
	using request_type
		= boost::beast::http::request< boost::beast::http::string_body >;


}


#endif
