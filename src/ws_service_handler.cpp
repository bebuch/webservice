//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <webservice/ws_service_handler.hpp>
#include <webservice/server.hpp>

#include "ws_session.hpp"

#include <thread>


namespace webservice{


	/// \brief Implementation of ws_service_handler
	struct ws_service_handler_impl{
		/// \brief Constructor
		ws_service_handler_impl(class server& server)
			: locker_([]()noexcept{})
			, run_lock_(locker_.first_lock())
			, strand_(server.get_executor()) {}


		/// \brief Protectes async operations
		async_locker locker_;

		/// \brief Keep one async operation alive until shutdown
		async_locker::lock run_lock_;

		/// \brief Run async operation sequential
		boost::asio::strand< boost::asio::io_context::executor_type > strand_;

		/// \brief Map from service name to object
		std::map< std::string, std::unique_ptr< ws_handler_base > > services;
	};


	ws_service_handler::ws_service_handler() = default;

	ws_service_handler::~ws_service_handler(){
		if(!impl_) return;

		server()->poll_while([this]()noexcept{
			return impl_->locker_.count() > 0;
		});
	}


	void ws_service_handler::set_server(class server& server){
		ws_handler_base::set_server(server);
		impl_ = std::make_unique< ws_service_handler_impl >(server);
	}

	void ws_service_handler::async_emplace(
		boost::asio::ip::tcp::socket&& socket,
		http_request&& req
	){
		assert(impl_ != nullptr);

		impl_->strand_.post(
			[
				this,
				lock = locker_.lock("ws_service_handler::async_emplace"),
				socket = std::move(socket),
				req = std::move(req)
			]()mutable{
				lock.enter();

				std::string name(req.target());
				auto iter = impl_->services.find(name);
				if(iter != impl_->services.end()){
					iter->second->async_emplace(
						std::move(socket), std::move(req));
				}else{
					throw std::logic_error("service(" + name
						+ ") doesn't exist");
				}
			}, std::allocator< void >());
	}


	void ws_service_handler::add_service(
		std::string name,
		std::unique_ptr< class ws_handler_base > service
	){
		assert(impl_ != nullptr);

		impl_->strand_.post(
			[
				this,
				lock = locker_.lock("ws_service_handler::add_service"),
				name = std::move(name),
				service = std::move(service)
			]()mutable{
				lock.enter();

				auto r = impl_->services.emplace(std::move(name), std::move(service));
				if(r.second){
					r.first->second->set_server(*server());
				}else{
					throw std::logic_error("service(" + name + ") already exists");
				}
			}, std::allocator< void >());
	}

	void ws_service_handler::erase_service(std::string name){
		assert(impl_ != nullptr);

		impl_->strand_.post(
			[
				this,
				lock = locker_.lock("ws_service_handler::erase_service"),
				name = std::move(name)
			]()mutable{
				lock.enter();

// 				// Services are erased by destructor if shutdown is active
// 				if(is_shutdown()) return;

				auto iter = impl_->services.find(name);
				if(iter != impl_->services.end()){
					iter->second->shutdown();
					impl_->services.erase(iter);
				}else{
					throw std::logic_error("service(" + name
						+ ") doesn't exist");
				}
			}, std::allocator< void >());
	}

	void ws_service_handler::on_shutdown()noexcept{
		assert(impl_ != nullptr);

		impl_->strand_.post(
			[
				this,
				lock = locker_.lock("ws_service_handler::on_shutdown")
			]()mutable{
				lock.enter();

				for(auto& service: impl_->services){
					service.second->shutdown();
				}
			}, std::allocator< void >());
	}


}
