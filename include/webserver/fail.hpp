//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webserver
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webserver__fail__hpp_INCLUDED_
#define _webserver__fail__hpp_INCLUDED_

#include <boost/system/error_code.hpp>

#include <iostream>


namespace webserver{


	// Report a failure
	void fail(boost::system::error_code ec, char const* what){
		std::cerr << what << ": " << ec.message() << "\n";
	}


}


#endif
