//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <webservice/ws_handler_base.hpp>

#include "ws_session.hpp"


namespace webservice{


	ws_handler_base::~ws_handler_base() = default;


	void ws_handler_base::on_open(
		ws_server_session* /*session*/,
		std::string const& /*resource*/){}

	void ws_handler_base::on_close(
		ws_server_session* /*session*/,
		std::string const& /*resource*/){}

	void ws_handler_base::on_text(
		ws_server_session* /*session*/,
		std::string const& /*resource*/,
		boost::beast::multi_buffer const& /*buffer*/){}

	void ws_handler_base::on_binary(
		ws_server_session* /*session*/,
		std::string const& /*resource*/,
		boost::beast::multi_buffer const& /*buffer*/){}

	void ws_handler_base::on_error(
		ws_server_session* /*session*/,
		std::string const& /*resource*/,
		ws_handler_location /*location*/,
		boost::system::error_code /*ec*/){}

	void ws_handler_base::on_exception(
		ws_server_session* /*session*/,
		std::string const& /*resource*/,
		std::exception_ptr /*error*/)noexcept{}


	void ws_handler_base::send_text(
		ws_server_session* session,
		shared_const_buffer buffer
	){
		session->send(std::make_tuple(text_tag{}, std::move(buffer)));
	}

	void ws_handler_base::send_binary(
		ws_server_session* session,
		shared_const_buffer buffer
	){
		session->send(std::make_tuple(binary_tag{}, std::move(buffer)));
	}

	void ws_handler_base::close(
		ws_server_session* session,
		boost::beast::string_view reason
	){
		session->send(boost::beast::websocket::close_reason(reason));
	}


	void ws_handler_base::set_server(class server& server)noexcept{
		server_ = &server;
	}


}
