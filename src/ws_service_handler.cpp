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
			, run_lock_(locker_.make_first_lock("ws_service_handler_impl"))
			, strand_(server.get_executor()) {}


		/// \brief Protectes async operations
		async_locker locker_;

		/// \brief Keep one async operation alive until shutdown
		async_locker::lock run_lock_;

		/// \brief Keep one async operation alive until last async finished
		async_locker::lock shutdown_lock_;

		/// \brief Run async operation sequential
		boost::asio::strand< boost::asio::io_context::executor_type > strand_;

		/// \brief Map from service name to object
		std::map< std::string, std::unique_ptr< ws_handler_base > > services;


		/// \brief true if on_shutdown was called
		bool is_shutdown()noexcept{
			return !run_lock_.is_locked();
		}

	};


	ws_service_handler::ws_service_handler() = default;

	ws_service_handler::~ws_service_handler(){
		if(!impl_) return;

		server()->poll_while([this]()noexcept{
			return impl_->locker_.count() > 0;
		});
	}


	void ws_service_handler::add_service(
		std::string name,
		std::unique_ptr< class ws_handler_base > service
	){
		if(!impl_){
			throw std::logic_error("add_service without server");
		}

		impl_->strand_.dispatch(
			[
				this,
				lock = impl_->locker_.make_lock("ws_service_handler::add_service"),
				name = std::move(name),
				service = std::move(service)
			]()mutable{
				lock.enter();

				auto r = impl_->services.emplace(std::move(name),
					std::move(service));
				if(r.second){
					r.first->second->set_server(*server());
				}else{
					throw std::logic_error("service(" + name
						+ ") already exists");
				}
			}, std::allocator< void >());
	}

	void ws_service_handler::erase_service(std::string name){
		if(!impl_){
			throw std::logic_error("erase_service without server");
		}

		impl_->strand_.dispatch(
			[
				this,
				lock = impl_->locker_.make_lock("ws_service_handler::erase_service"),
				name = std::move(name)
			]()mutable{
				lock.enter();

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


	void ws_service_handler::on_server(class server& server){
		impl_ = std::make_unique< ws_service_handler_impl >(server);
	}

	void ws_service_handler::on_make(
		boost::asio::ip::tcp::socket&& socket,
		http_request&& req
	){
		assert(impl_ != nullptr);

		impl_->strand_.dispatch(
			[
				this,
				lock = impl_->locker_.make_lock("ws_service_handler::on_make"),
				socket = std::move(socket),
				req = std::move(req)
			]()mutable{
				lock.enter();

				std::string name(req.target());
				auto iter = impl_->services.find(name);
				if(iter != impl_->services.end()){
					iter->second->make(std::move(socket), std::move(req));

					if(impl_->services.empty() && is_shutdown()){
						impl_->shutdown_lock_.unlock();
					}
				}else{
					throw std::logic_error("service(" + name
						+ ") doesn't exist");
				}
			}, std::allocator< void >());

	}

	void ws_service_handler::on_shutdown()noexcept{
		assert(impl_ != nullptr);

		auto lock = std::move(impl_->run_lock_);
		if(lock.is_locked()){
			impl_->shutdown_lock_ = std::move(lock);

			impl_->strand_.defer(
				[
					this,
					lock = impl_->locker_.make_lock("ws_service_handler::on_shutdown")
				]()mutable{
					lock.enter();

					if(impl_->services.empty()){
						impl_->shutdown_lock_.unlock();
					}else{
						for(auto& service: impl_->services){
							service.second->shutdown();
						}
					}
				}, std::allocator< void >());
		}
	}



}
