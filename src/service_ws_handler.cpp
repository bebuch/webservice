//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <webservice/service_ws_handler.hpp>
#include <webservice/ws_service_base.hpp>

#include "ws_session.hpp"

#include <shared_mutex>


namespace webservice{


	struct service_ws_handler_impl{
		std::shared_timed_mutex mutex;
		std::map< std::string, std::unique_ptr< ws_service_base > > services;

		ws_service_base* find_service(std::string const& resource)const{
			auto iter = services.find(resource);
			if(iter != services.end()){
				return iter->second.get();
			}else{
				return nullptr;
			}
		}
	};


	service_ws_handler::service_ws_handler()
		: impl_(std::make_unique< service_ws_handler_impl >()){}

	service_ws_handler::~service_ws_handler() = default;


	void service_ws_handler::add_service(
		std::string name,
		std::unique_ptr< class ws_service_base > service
	){
		std::unique_lock< std::shared_timed_mutex > lock(impl_->mutex);
	}

	void service_ws_handler::erase_service(std::string name){
		std::unique_lock< std::shared_timed_mutex > lock(impl_->mutex);
	}


	void service_ws_handler::on_open(
		ws_server_session* session,
		std::string const& resource
	){
		std::shared_lock< std::shared_timed_mutex > lock(impl_->mutex);
		auto service = impl_->find_service(resource);
		if(service == nullptr){
			return;
		}
		service->on_open(session);
	}

	void service_ws_handler::on_close(
		ws_server_session* session,
		std::string const& resource
	){
		std::shared_lock< std::shared_timed_mutex > lock(impl_->mutex);
		auto service = impl_->find_service(resource);
		if(service == nullptr){
			return;
		}
		service->on_close(session);
	}

	void service_ws_handler::on_text(
		ws_server_session* session,
		std::string const& resource,
		boost::beast::multi_buffer const& buffer
	){
		std::shared_lock< std::shared_timed_mutex > lock(impl_->mutex);
		auto service = impl_->find_service(resource);
		if(service == nullptr){
			return;
		}
		service->on_text(session, buffer);
	}

	void service_ws_handler::on_binary(
		ws_server_session* session,
		std::string const& resource,
		boost::beast::multi_buffer const& buffer
	){
		std::shared_lock< std::shared_timed_mutex > lock(impl_->mutex);
		auto service = impl_->find_service(resource);
		if(service == nullptr){
			return;
		}
		service->on_binary(session, buffer);
	}

	void service_ws_handler::on_error(
		ws_server_session* session,
		std::string const& resource,
		ws_handler_location location,
		boost::system::error_code ec
	){
		std::shared_lock< std::shared_timed_mutex > lock(impl_->mutex);
		auto service = impl_->find_service(resource);
		if(service == nullptr){
			return;
		}
		service->on_error(session, location, ec);
	}

	void service_ws_handler::on_exception(
		ws_server_session* session,
		std::string const& resource,
		std::exception_ptr error
	)noexcept{
		std::shared_lock< std::shared_timed_mutex > lock(impl_->mutex);
		auto service = impl_->find_service(resource);
		if(service == nullptr){
			return;
		}
		service->on_exception(session, error);
	}


}
