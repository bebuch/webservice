//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__json_ws_handler__hpp_INCLUDED_
#define _webservice__json_ws_handler__hpp_INCLUDED_

#include "json_conversion.hpp"
#include "basic_ws_handler.hpp"


namespace webservice{


	template <
		typename SendBinaryType,
		typename ReceiveBinaryType = SendBinaryType >
	class basic_json_ws_handler
		: public basic_ws_handler<
			nlohmann::json, SendBinaryType, nlohmann::json, ReceiveBinaryType >
	{
		using basic_ws_handler< nlohmann::json, SendBinaryType,
			nlohmann::json, ReceiveBinaryType >::basic_ws_handler;
	};

	class json_ws_handler
		: public basic_json_ws_handler< std::vector< std::uint8_t > >
	{
		using basic_json_ws_handler::basic_json_ws_handler;
	};


}


#endif
