//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webserver
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webserver__mime_type__hpp_INCLUDED_
#define _webserver__mime_type__hpp_INCLUDED_

#include <boost/beast/core/string.hpp>

#include <map>


namespace webserver{


	/// \brief Return a reasonable mime type based on the file extension
	boost::beast::string_view mime_type(boost::beast::string_view path);


}


#endif
