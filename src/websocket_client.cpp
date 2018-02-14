//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <webservice/websocket_client.hpp>

#include "websocket_client_impl.hpp"


namespace webservice{


	class websocket_client_impl{};


	websocket_client::websocket_client(
		boost::asio::ip::address address,
		std::uint16_t port,
		std::string resource
	)
		: impl_(boost::make_unique< websocket_client_impl >(
			address, port, std::move(resource))) {}

	websocket_client::~websocket_client(){}


	void websocket_client::send_text(std::string data){
		impl_->send_text(data);
	}

	void websocket_client::send_binary(std::vector< std::uint8_t > data){
		impl_->send_binary(data);
	}

	void websocket_client::close(boost::beast::string_view reason){
		impl_->close(reason);
	}


	void websocket_client::on_open(){}

	void websocket_client::on_close(boost::beast::string_view /*reason*/){}

	void websocket_client::on_text(boost::beast::multi_buffer& /*buffer*/){}

	void websocket_client::on_binary(boost::beast::multi_buffer& /*buffer*/){}


}
