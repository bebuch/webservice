//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__file_request_handler__hpp_INCLUDED_
#define _webservice__file_request_handler__hpp_INCLUDED_

#include "http_request_handler.hpp"

#include <boost/beast/core.hpp>
#include <boost/beast/version.hpp>


namespace webservice{


	class file_request_handler: public http_request_handler{
	public:
		file_request_handler(std::string doc_root)
			: doc_root_(std::move(doc_root)) {}

		void operator()(http_request&& req, http_response&& send)override;


		std::string const& doc_root()const;


	protected:

		virtual void on_file_not_found(
			http_request&& req,
			http_response&& send
		);

		void send_body(
			http_request&& req,
			http_response&& send,
			boost::beast::http::file_body::value_type&& body,
			boost::beast::string_view mime_type
		);


	private:
		std::string doc_root_;
	};


}


#endif
