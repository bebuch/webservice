//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include "ws_service_impl.hpp"


namespace webservice{


	ws_service::ws_service()
		: impl_(std::make_unique< ws_service_impl >(*this)){}


	ws_service::~ws_service(){}


	void ws_service::on_open(
		std::uintptr_t const /*identifier*/,
		std::string const& /*resource*/){}

	void ws_service::on_close(
		std::uintptr_t const /*identifier*/,
		std::string const& /*resource*/){}

	void ws_service::on_text(
		std::uintptr_t const /*identifier*/,
		std::string const& /*resource*/,
		boost::beast::multi_buffer const& /*buffer*/){}

	void ws_service::on_binary(
		std::uintptr_t const /*identifier*/,
		std::string const& /*resource*/,
		boost::beast::multi_buffer const& /*buffer*/){}

	void ws_service::on_error(
		std::uintptr_t /*identifier*/,
		std::string const& /*resource*/,
		ws_service_location /*location*/,
		boost::system::error_code /*ec*/){}

	void ws_service::on_exception(
		std::uintptr_t /*identifier*/,
		std::string const& /*resource*/,
		std::exception_ptr /*error*/)noexcept{}


	void ws_service::send_text(std::string data){
		impl_->send(std::make_shared< std::string >(std::move(data)));
	}

	void ws_service::send_text(
		std::uintptr_t const identifier,
		std::string data
	){
		impl_->send(
			identifier,
			std::make_shared< std::string >(std::move(data)));
	}

	void ws_service::send_text(
		std::set< std::uintptr_t > const& identifier,
		std::string data
	){
		impl_->send(
			identifier,
			std::make_shared< std::string >(std::move(data)));
	}


	void ws_service::send_binary(
		std::vector< std::uint8_t > data
	){
		impl_->send(
			std::make_shared< std::vector< std::uint8_t > >(std::move(data)));
	}

	void ws_service::send_binary(
		std::uintptr_t const identifier,
		std::vector< std::uint8_t > data
	){
		impl_->send(
			identifier,
			std::make_shared< std::vector< std::uint8_t > >(std::move(data)));
	}

	void ws_service::send_binary(
		std::set< std::uintptr_t > const& identifier,
		std::vector< std::uint8_t > data
	){
		impl_->send(
			identifier,
			std::make_shared< std::vector< std::uint8_t > >(std::move(data)));
	}


	void ws_service::close(boost::beast::string_view reason){
		impl_->send(boost::beast::websocket::close_reason(reason));
	}

	void ws_service::close(
		std::uintptr_t identifier,
		boost::beast::string_view reason
	){
		impl_->send(identifier, boost::beast::websocket::close_reason(reason));
	}

	void ws_service::close(
		std::set< std::uintptr_t > const& identifier,
		boost::beast::string_view reason
	){
		impl_->send(identifier, boost::beast::websocket::close_reason(reason));
	}


}
