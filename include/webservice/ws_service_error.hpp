//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__ws_service_error__hpp_INCLUDED_
#define _webservice__ws_service_error__hpp_INCLUDED_


namespace webservice{


	enum class ws_service_error{
		accept,
		read,
		write,
		timer,
		ping
	};


}


#endif
