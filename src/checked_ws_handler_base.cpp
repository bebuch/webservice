//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <webservice/checked_ws_handler_base.hpp>

#include "checked_ws_handler_base_impl.hpp"


namespace webservice{



	checked_ws_handler_base::checked_ws_handler_base()
		: impl_(std::make_unique< checked_ws_handler_base_impl >(*this)){}


	checked_ws_handler_base::~checked_ws_handler_base(){
		impl_->shutdown();
	}


	void checked_ws_handler_base::on_open(
		std::uintptr_t const /*identifier*/,
		std::string const& /*resource*/){}

	void checked_ws_handler_base::on_close(
		std::uintptr_t const /*identifier*/,
		std::string const& /*resource*/){}

	void checked_ws_handler_base::on_text(
		std::uintptr_t const /*identifier*/,
		std::string const& /*resource*/,
		boost::beast::multi_buffer const& /*buffer*/){}

	void checked_ws_handler_base::on_binary(
		std::uintptr_t const /*identifier*/,
		std::string const& /*resource*/,
		boost::beast::multi_buffer const& /*buffer*/){}

	void checked_ws_handler_base::on_error(
		std::uintptr_t const /*identifier*/,
		std::string const& /*resource*/,
		ws_handler_location /*location*/,
		boost::system::error_code /*ec*/){}

	void checked_ws_handler_base::on_exception(
		std::uintptr_t const /*identifier*/,
		std::string const& /*resource*/,
		std::exception_ptr /*error*/)noexcept{}


	void checked_ws_handler_base::on_open(
		ws_server_session* const session,
		std::string const& resource
	){
		impl_->on_open(session, resource);
	}

	void checked_ws_handler_base::on_close(
		ws_server_session* const session,
		std::string const& resource
	){
		impl_->on_close(session, resource);
	}

	void checked_ws_handler_base::on_text(
		ws_server_session* const session,
		std::string const& resource,
		boost::beast::multi_buffer const& buffer
	){
		impl_->on_text(session, resource, buffer);
	}

	void checked_ws_handler_base::on_binary(
		ws_server_session* const session,
		std::string const& resource,
		boost::beast::multi_buffer const& buffer
	){
		impl_->on_binary(session, resource, buffer);
	}

	void checked_ws_handler_base::on_error(
		ws_server_session* const session,
		std::string const& resource,
		ws_handler_location location,
		boost::system::error_code ec
	){
		impl_->on_error(session, resource, location, ec);
	}

	void checked_ws_handler_base::on_exception(
		ws_server_session* session,
		std::string const& resource,
		std::exception_ptr error
	)noexcept{
		impl_->on_exception(session, resource, error);
	}


	void checked_ws_handler_base::send_text(shared_const_buffer buffer){
		impl_->send(std::make_tuple(text_tag{}, std::move(buffer)));
	}

	void checked_ws_handler_base::send_text(
		std::uintptr_t const identifier,
		shared_const_buffer buffer
	){
		impl_->send(
			identifier,
			std::make_tuple(text_tag{}, std::move(buffer)));
	}

	void checked_ws_handler_base::send_text(
		std::set< std::uintptr_t > const& identifier,
		shared_const_buffer buffer
	){
		impl_->send(
			identifier,
			std::make_tuple(text_tag{}, std::move(buffer)));
	}


	void checked_ws_handler_base::send_binary(shared_const_buffer buffer){
		impl_->send(std::make_tuple(binary_tag{}, std::move(buffer)));
	}

	void checked_ws_handler_base::send_binary(
		std::uintptr_t const identifier,
		shared_const_buffer buffer
	){
		impl_->send(
			identifier,
			std::make_tuple(binary_tag{}, std::move(buffer)));
	}

	void checked_ws_handler_base::send_binary(
		std::set< std::uintptr_t > const& identifier,
		shared_const_buffer buffer
	){
		impl_->send(
			identifier,
			std::make_tuple(binary_tag{}, std::move(buffer)));
	}


	void checked_ws_handler_base::close(boost::beast::string_view reason){
		impl_->send(boost::beast::websocket::close_reason(reason));
	}

	void checked_ws_handler_base::close(
		std::uintptr_t identifier,
		boost::beast::string_view reason
	){
		impl_->send(identifier, boost::beast::websocket::close_reason(reason));
	}

	void checked_ws_handler_base::close(
		std::set< std::uintptr_t > const& identifier,
		boost::beast::string_view reason
	){
		impl_->send(identifier, boost::beast::websocket::close_reason(reason));
	}



}
