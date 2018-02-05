//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webserver
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webserver__file_request_handler__hpp_INCLUDED_
#define _webserver__file_request_handler__hpp_INCLUDED_

#include "http_request_handler.hpp"
#include "path_concat.hpp"
#include "mime_type.hpp"

#include <boost/beast/core.hpp>
#include <boost/beast/version.hpp>


namespace webserver{


	class file_request_handler: public http_request_handler{
	public:
		file_request_handler(std::string doc_root)
			: doc_root_(std::move(doc_root)) {}

		virtual void operator()(
			http_request&& req, http_response&& send)override;


	private:
		std::string doc_root_;
	};


}


#endif
