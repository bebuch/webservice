//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webserver
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webserver__path_concat__hpp_INCLUDED_
#define _webserver__path_concat__hpp_INCLUDED_

#include <boost/beast/core/string.hpp>


namespace webserver{


	/// \brief Append an HTTP rel-path to the local filesystem path base
	std::string path_concat(
		boost::beast::string_view base,
		boost::beast::string_view path);


}


#endif
