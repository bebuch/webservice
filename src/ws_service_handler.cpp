//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <webservice/ws_service_handler.hpp>

#include "ws_session.hpp"

#include <shared_mutex>


namespace webservice{


	struct ws_service_handler_impl{
		std::shared_timed_mutex mutex;
		std::map< std::string, std::unique_ptr< ws_handler_base > > services;

		ws_handler_base* find_service(std::string const& resource)const{
			auto iter = services.find(resource);
			if(iter != services.end()){
				return iter->second.get();
			}else{
				return nullptr;
			}
		}
	};


	ws_service_handler::ws_service_handler()
		: impl_(std::make_unique< ws_service_handler_impl >()){}

	ws_service_handler::~ws_service_handler() = default;


	void ws_service_handler::add_service(
		std::string name,
		std::unique_ptr< class ws_handler_base > service
	){
		std::unique_lock< std::shared_timed_mutex > lock(impl_->mutex);
		auto r = impl_->services.emplace(std::move(name), std::move(service));
		if(r.second){
			r.first->second->set_server(server());
		}else{
			throw std::logic_error("service '" + name + "' already exists");
		}
	}

	void ws_service_handler::erase_service(std::string name){
		std::unique_lock< std::shared_timed_mutex > lock(impl_->mutex);
		auto iter = impl_->services.find(name);
		if(iter != impl_->services.end()){
			impl_->services.erase(iter);
		}else{
			throw std::logic_error("service '" + name + "' doesn't exist");
		}
	}


	void ws_service_handler::on_open(
		ws_server_session* session,
		std::string const& resource
	){
		std::shared_lock< std::shared_timed_mutex > lock(impl_->mutex);
		auto service = impl_->find_service(resource);
		if(service == nullptr){
			on_unknown_service(session, resource);
		}else{
			try{
				session->rebind(*service);
				service->on_open(session, resource);
			}catch(...){
				service->on_exception(session, resource, std::current_exception());
			}
		}
	}

	void ws_service_handler::on_unknown_service(
		ws_server_session* session,
		std::string const& resource
	){
		try{
			session->send(boost::beast::websocket::close_reason(
				"unknown target: " + resource));
		}catch(...){
			on_exception(session, resource, std::current_exception());
		}
	}


}
