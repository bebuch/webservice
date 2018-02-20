// //-----------------------------------------------------------------------------
// // Copyright (c) 2018 Benjamin Buch
// //
// // https://github.com/bebuch/webservice
// //
// // Distributed under the Boost Software License, Version 1.0. (See accompanying
// // file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
// //-----------------------------------------------------------------------------
// #include <webservice/websocket_client.hpp>
//
// #include <boost/make_unique.hpp>
//
// #include "websocket_client_impl.hpp"
//
//
// namespace webservice{
//
//
// 	class websocket_client_impl;
//
//
// 	websocket_client::websocket_client(
// 		std::string host,
// 		std::uint16_t port,
// 		std::string resource
// 	)
// 		: impl_(boost::make_unique< websocket_client_impl >(
// 			*this, std::move(host), port, std::move(resource))) {}
//
// 	websocket_client::~websocket_client(){}
//
//
// 	void websocket_client::send_text(std::string data){
// 		impl_->send(std::make_shared< std::string >(std::move(data)));
// 	}
//
// 	void websocket_client::send_binary(std::vector< std::uint8_t > data){
// 		impl_->send(std::make_shared< std::vector< std::uint8_t > >(
// 			std::move(data)));
// 	}
//
// 	void websocket_client::close(boost::beast::string_view reason){
// 		impl_->close(reason);
// 	}
//
//
// 	void websocket_client::on_open(){}
//
// 	void websocket_client::on_error(boost::system::error_code /*ec*/){}
//
// 	void websocket_client::on_close(boost::beast::string_view /*reason*/){}
//
// 	void websocket_client::on_text(boost::beast::multi_buffer& /*buffer*/){}
//
// 	void websocket_client::on_binary(boost::beast::multi_buffer& /*buffer*/){}
//
// 	void websocket_client::on_exception(std::exception_ptr /*error*/)noexcept{}
//
//
// }
