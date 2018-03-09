//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <webservice/checked_ws_service_base.hpp>


namespace webservice{


	checked_ws_service_base::~checked_ws_service_base() = default;


	void checked_ws_service_base::send_text(shared_const_buffer buffer){

	}

	void checked_ws_service_base::send_text(
		std::uintptr_t identifier,
		shared_const_buffer buffer
	){

	}

	void checked_ws_service_base::send_text(
		std::set< std::uintptr_t > const& identifier,
		shared_const_buffer buffer
	){

	}


	void checked_ws_service_base::send_binary(shared_const_buffer buffer){

	}

	void checked_ws_service_base::send_binary(
		std::uintptr_t identifier,
		shared_const_buffer buffer
	){

	}

	void checked_ws_service_base::send_binary(
		std::set< std::uintptr_t > const& identifier,
		shared_const_buffer buffer
	){

	}


	void checked_ws_service_base::close(boost::beast::string_view reason){

	}

	void checked_ws_service_base::close(
		std::uintptr_t identifier,
		boost::beast::string_view reason
	){

	}

	void checked_ws_service_base::close(
		std::set< std::uintptr_t > const& identifier,
		boost::beast::string_view reason
	){

	}


	void checked_ws_service_base::on_open(std::uintptr_t identifier){}

	void checked_ws_service_base::on_close(std::uintptr_t identifier){}

	void checked_ws_service_base::on_text(
		std::uintptr_t identifier,
		boost::beast::multi_buffer const& buffer){}

	void checked_ws_service_base::on_binary(
		std::uintptr_t identifier,
		boost::beast::multi_buffer const& buffer){}

	void checked_ws_service_base::on_error(
		std::uintptr_t identifier,
		ws_handler_location location,
		boost::system::error_code ec){}

	void checked_ws_service_base::on_exception(
		std::uintptr_t identifier,
		std::exception_ptr error)noexcept{}



}
