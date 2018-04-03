//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include "server_impl.hpp"

#include <webservice/http_request_handler.hpp>
#include <webservice/ws_handler_base.hpp>
#include <webservice/error_handler.hpp>


namespace webservice{


	server_impl::server_impl(
		class server& server,
		std::unique_ptr< http_request_handler >&& handler,
		std::unique_ptr< ws_handler_base >&& service,
		std::unique_ptr< error_handler >&& error_handler,
		boost::asio::ip::address const address,
		std::uint16_t const port,
		std::uint8_t const thread_count
	)
		: server_(server)
		, handler_([&server, handler = std::move(handler)]()mutable{
				if(!handler){
					handler =
						std::make_unique< http_request_handler >();
				}
				handler->set_server(server);
				return std::move(handler);
			}())
		, service_([&server, service = std::move(service)]()mutable{
				if(service){
					service->set_server(server);
				}
				return std::move(service);
			}())
		, error_handler_([error_handler = std::move(error_handler)]()mutable{
				if(!error_handler){
					error_handler =
						std::make_unique< class error_handler >();
				}
				return std::move(error_handler);
			}())
		, ioc_{thread_count}
		, listener_(*this, boost::asio::ip::tcp::endpoint{address, port}, ioc_)
	{
		// Run the I/O service on the requested number of thread_count
		threads_.reserve(thread_count);
		for(std::size_t i = 0; i < thread_count; ++i){
			threads_.emplace_back([this]{
				// restart io_context if it returned by exception
				for(;;){
					try{
						ioc_.run();
						return;
					}catch(...){
						error().on_exception(std::current_exception());
					}
				}
			});
		}
	}


	void server_impl::block()noexcept{
		std::lock_guard< std::recursive_mutex > lock(mutex_);
		for(auto& thread: threads_){
			if(thread.joinable()){
				try{
					thread.join();
				}catch(...){
					error().on_exception(std::current_exception());
				}
			}
		}
	}

	void server_impl::shutdown()noexcept{
		listener_.shutdown();
	}

	boost::asio::executor server_impl::get_executor(){
		return ioc_.get_executor();
	}

	std::size_t server_impl::poll_one()noexcept{
		try{
			return ioc_.poll_one();
		}catch(...){
			error().on_exception(std::current_exception());
			return 1;
		}
	}


}
