//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include "server_impl.hpp"

#include <webservice/server.hpp>

#include <thread>
#include <mutex>


namespace webservice{


	server::server(
		std::unique_ptr< http_request_handler > handler,
		std::unique_ptr< ws_handler_base > service,
		std::unique_ptr< error_handler > error_handler,
		boost::asio::ip::address const address,
		std::uint16_t const port,
		std::uint8_t const thread_count
	)
		: ioc_{thread_count}
		, impl_(std::make_unique< server_impl >(
				*this,
				std::move(handler),
				std::move(service),
				std::move(error_handler),
				address,
				port,
				thread_count
			)) {}

	server::~server(){
		shutdown();
		block();
	}


	void server::block()noexcept{
		impl_->block();
	}

	void server::shutdown()noexcept{
		impl_->shutdown();
	}

	boost::asio::io_context::executor_type server::get_executor(){
		return ioc_.get_executor();
	}

	boost::asio::io_context& server::get_io_context()noexcept{
		return ioc_;
	}

	void server::poll_while(
		std::atomic< std::size_t > const& async_calls
	)noexcept{
		// As long as async calls are pending
		while(async_calls > 0){
			try{
				// Request the client to run a handler async
				if(ioc_.poll_one() == 0){
					// If no handler was waiting, the pending one must
					// currently run in another thread
					std::this_thread::yield();
				}
			}catch(...){
				impl().error().on_exception(std::current_exception());
			}
		}
	}

	server_impl& server::impl()noexcept{
		return *impl_;
	}


}
