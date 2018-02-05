//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webserver
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <webserver/http_request_handler.hpp>

#include <boost/beast/version.hpp>


namespace webserver{


	namespace http = boost::beast::http;


	void http_request_handler::operator()(
		http_request&& req,
		http_response&& send
	){
		send(not_found(req, req.target()));
	}


	http_string_response bad_request(
		http_request const& req,
		boost::beast::string_view why
	){
		http_string_response res{http::status::bad_request, req.version()};
		res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
		res.set(http::field::content_type, "text/html");
		res.keep_alive(req.keep_alive());
		res.body() = why.to_string();
		res.prepare_payload();
		return res;
	}

	http_string_response not_found(
		http_request const& req,
		boost::beast::string_view target
	){
		http_string_response res{http::status::not_found, req.version()};
		res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
		res.set(http::field::content_type, "text/html");
		res.keep_alive(req.keep_alive());
		res.body() = "The resource '" + target.to_string() + "' was not found.";
		res.prepare_payload();
		return res;
	}

	http_string_response server_error(
		http_request const& req,
		boost::beast::string_view what
	){
		http_string_response res{
			http::status::internal_server_error, req.version()};
		res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
		res.set(http::field::content_type, "text/html");
		res.keep_alive(req.keep_alive());
		res.body() = "An error occurred: '" + what.to_string() + "'";
		res.prepare_payload();
		return res;
	}


}
