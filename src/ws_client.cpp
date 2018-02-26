//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <webservice/ws_client.hpp>

#include <boost/make_unique.hpp>

#include "ws_client_impl.hpp"


namespace webservice{


	class ws_client_impl;


	ws_client::ws_client(
		std::string host,
		std::string port,
		std::string resource
	)
		: impl_(boost::make_unique< ws_client_impl >(
			*this, std::move(host), std::move(port), std::move(resource))) {}

	ws_client::~ws_client(){}


	void ws_client::connect(){
		impl_->connect();
	}

	bool ws_client::is_connected()const{
		return impl_->is_connected();
	}


	void ws_client::send_text(std::string data){
		impl_->send(std::make_shared< std::string >(std::move(data)));
	}

	void ws_client::send_binary(std::vector< std::uint8_t > data){
		impl_->send(std::make_shared< std::vector< std::uint8_t > >(
			std::move(data)));
	}


	void ws_client::close(boost::beast::string_view reason){
		impl_->send(boost::beast::websocket::close_reason(reason));
		impl_->close();
	}


	void ws_client::on_open(){}

	void ws_client::on_close(){}

	void ws_client::on_text(boost::beast::multi_buffer& /*buffer*/){}

	void ws_client::on_binary(boost::beast::multi_buffer& /*buffer*/){}

	void ws_client::on_error(
		ws_client_error /*error*/,
		boost::system::error_code /*ec*/){}

	void ws_client::on_exception(std::exception_ptr /*error*/)noexcept{}


}
