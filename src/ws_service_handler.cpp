//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <webservice/ws_service_handler.hpp>
#include <webservice/async_lock.hpp>
#include <webservice/executor.hpp>

#include <boost/asio/strand.hpp>


namespace webservice{


	/// \brief Implementation of ws_service_handler
	struct ws_service_handler_impl{
		/// \brief Constructor
		ws_service_handler_impl(class executor& executor)
			: locker_([]()noexcept{}) // TODO: Call erase
			, run_lock_(locker_.make_first_lock("ws_service_handler_impl"))
			, strand_(executor.get_executor()) {}


		/// \brief Protectes async operations
		async_locker locker_;

		/// \brief Keep one async operation alive until shutdown
		async_locker::lock run_lock_;

		/// \brief Keep one async operation alive until last async finished
		async_locker::lock shutdown_lock_;

		/// \brief Run async operation sequential
		boost::asio::strand< boost::asio::io_context::executor_type > strand_;

		/// \brief Map type from service name to object
		using service_map =
			std::map< std::string, std::unique_ptr< ws_service_interface > >;

		/// \brief Map from service name to object
		service_map services_;
	};


	ws_service_handler::ws_service_handler() = default;

	ws_service_handler::~ws_service_handler() = default;


	void ws_service_handler::add_service(
		std::string name,
		std::unique_ptr< ws_service_interface > service
	){
		if(!impl_){
			throw std::logic_error(
				"called add_service() before server was set");
		}

		impl_->strand_.dispatch(
			[
				this,
				lock = impl_->locker_.make_lock("ws_service_handler::add_service"),
				name = std::move(name),
				service = std::move(service)
			]()mutable noexcept{
				try{
					lock.enter();

					auto r = impl_->services_.emplace(std::move(name),
						std::move(service));
					if(r.second){
						r.first->second->set_executor(executor());
					}else{
						throw std::logic_error("service(" + name
							+ ") already exists");
					}
				}catch(...){
					on_exception(std::current_exception());
				}
			}, std::allocator< void >());
	}

	void ws_service_handler::erase_service(std::string name){
		if(!impl_){
			throw std::logic_error(
				"called erase_service() before server was set");
		}

		impl_->strand_.dispatch(
			[
				this,
				lock = impl_->locker_.make_lock("ws_service_handler::erase_service"),
				name = std::move(name)
			]()mutable noexcept{
				try{
					lock.enter();

					auto iter = impl_->services_.find(name);
					if(iter != impl_->services_.end()){
						iter->second->shutdown();
						impl_->services_.erase(iter);
					}else{
						throw std::logic_error("service(" + name
							+ ") doesn't exist");
					}
				}catch(...){
					on_exception(std::current_exception());
				}
			}, std::allocator< void >());
	}


	bool ws_service_handler::is_shutdown()noexcept{
		return impl_ && !impl_->run_lock_.is_locked();
	}


	void ws_service_handler::on_executor(){
		impl_ = std::make_unique< ws_service_handler_impl >(executor());
	}

	void ws_service_handler::on_server_connect(
		boost::asio::ip::tcp::socket&& socket,
		http_request&& req
	){
		assert(impl_ != nullptr);

		impl_->strand_.dispatch(
			[
				this,
				lock = impl_->locker_.make_lock("ws_service_handler::on_server_connect"),
				socket = std::move(socket),
				req = std::move(req)
			]()mutable noexcept{
				try{
					lock.enter();

					std::string name(req.target());
					auto iter = impl_->services_.find(name);
					if(iter != impl_->services_.end()){
						iter->second->server_connect(std::move(socket),
							std::move(req));

						if(impl_->services_.empty() && is_shutdown()){
							impl_->shutdown_lock_.unlock();
						}
					}else{
						throw std::logic_error("service(" + name
							+ ") doesn't exist");
					}
				}catch(...){
					on_exception(std::current_exception());
				}
			}, std::allocator< void >());
	}

	void ws_service_handler::on_client_connect(
		std::string&& host,
		std::string&& port,
		std::string&& resource
	){
		assert(impl_ != nullptr);

		impl_->strand_.dispatch(
			[
				this,
				lock = impl_->locker_.make_lock("ws_service_handler::on_client_connect"),
				host = std::move(host),
				port = std::move(port),
				resource = std::move(resource)
			]()mutable noexcept{
				try{
					lock.enter();

					auto iter = impl_->services_.find(resource);
					if(iter != impl_->services_.end()){
						iter->second->client_connect(std::move(host),
							std::move(port), std::move(resource));

						if(impl_->services_.empty() && is_shutdown()){
							impl_->shutdown_lock_.unlock();
						}
					}else{
						throw std::logic_error("service(" + resource
							+ ") doesn't exist");
					}
				}catch(...){
					on_exception(std::current_exception());
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
				]()mutable noexcept{
					try{
						lock.enter();

						if(impl_->services_.empty()){
							impl_->shutdown_lock_.unlock();
						}else{
							for(auto& service: impl_->services_){
								service.second->shutdown();
							}
						}
					}catch(...){
						on_exception(std::current_exception());
					}
				}, std::allocator< void >());
		}
	}



}
