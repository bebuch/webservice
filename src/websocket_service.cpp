//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include "websocket_service_impl.hpp"


namespace webservice{


	websocket_service::websocket_service()
		: impl_(std::make_unique< websocket_service_impl >(*this)){}


	websocket_service::~websocket_service(){}


	void websocket_service::on_open(
		std::uintptr_t const /*identifier*/,
		std::string const& /*resource*/){}

	void websocket_service::on_close(
		std::uintptr_t const /*identifier*/,
		std::string const& /*resource*/){}

	void websocket_service::on_text(
		std::uintptr_t const /*identifier*/,
		std::string const& /*resource*/,
		boost::beast::multi_buffer& /*buffer*/){}

	void websocket_service::on_binary(
		std::uintptr_t const /*identifier*/,
		std::string const& /*resource*/,
		boost::beast::multi_buffer& /*buffer*/){}

	void websocket_service::on_error(
		std::uintptr_t /*identifier*/,
		std::string const& /*resource*/,
		websocket_service_error /*error*/,
		boost::system::error_code /*ec*/){}

	void websocket_service::on_exception(
		std::uintptr_t /*identifier*/,
		std::string const& /*resource*/,
		std::exception_ptr /*error*/)noexcept{}


	void websocket_service::send_text(std::string data){
		impl_->send(std::make_shared< std::string >(std::move(data)));
	}

	void websocket_service::send_text(
		std::uintptr_t const identifier,
		std::string data
	){
		impl_->send(
			identifier,
			std::make_shared< std::string >(std::move(data)));
	}

	void websocket_service::send_text(
		std::set< std::uintptr_t > const& identifier,
		std::string data
	){
		impl_->send(
			identifier,
			std::make_shared< std::string >(std::move(data)));
	}


	void websocket_service::send_binary(
		std::vector< std::uint8_t > data
	){
		impl_->send(
			std::make_shared< std::vector< std::uint8_t > >(std::move(data)));
	}

	void websocket_service::send_binary(
		std::uintptr_t const identifier,
		std::vector< std::uint8_t > data
	){
		impl_->send(
			identifier,
			std::make_shared< std::vector< std::uint8_t > >(std::move(data)));
	}

	void websocket_service::send_binary(
		std::set< std::uintptr_t > const& identifier,
		std::vector< std::uint8_t > data
	){
		impl_->send(
			identifier,
			std::make_shared< std::vector< std::uint8_t > >(std::move(data)));
	}


	void websocket_service::close(boost::beast::string_view reason){
		impl_->send(boost::beast::websocket::close_reason(reason));
	}

	void websocket_service::close(
		std::uintptr_t identifier,
		boost::beast::string_view reason
	){
		impl_->send(identifier, boost::beast::websocket::close_reason(reason));
	}

	void websocket_service::close(
		std::set< std::uintptr_t > const& identifier,
		boost::beast::string_view reason
	){
		impl_->send(identifier, boost::beast::websocket::close_reason(reason));
	}


}
