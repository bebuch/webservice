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
		: impl_(std::make_unique< server_impl >(
				[this, handler = std::move(handler)]()mutable{
						if(!handler){
							handler =
								std::make_unique< http_request_handler >();
						}
						handler->set_server(this);
						return std::move(handler);
					}(),
				[this, service = std::move(service)]()mutable{
						if(service){
							service->set_server(this);
						}
						return std::move(service);
					}(),
				[error_handler = std::move(error_handler)]()mutable{
						if(!error_handler){
							error_handler =
								std::make_unique< class error_handler >();
						}
						return std::move(error_handler);
					}(),
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

	boost::asio::executor server::get_executor(){
		return impl_->get_executor();
	}

	std::size_t server::poll_one()noexcept{
		return impl_->poll_one();
	}


}
