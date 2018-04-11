//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include "http_sessions.hpp"
#include "http_session.hpp"

#include <webservice/server.hpp>

#include <thread>


namespace webservice{


	http_sessions::http_sessions(class server& server)
		: server_(server)
		, strand_(server_.get_executor()) {}


	class server* http_sessions::server()const noexcept{
		return &server_;
	}


	void http_sessions::async_emplace(
		boost::asio::ip::tcp::socket&& socket,
		http_request_handler& handler
	){
		strand_.post(
			[
				this,
				lock = async_lock(async_calls_),
				socket = std::move(socket),
				&handler
			]()mutable{
				if(shutdown_){
					throw std::logic_error(
						"emplace in http_sessions while shutdown");
				}

				auto session = std::make_unique< http_session >(
					std::move(socket), handler);

				auto iter = set_.insert(set_.end(), std::move(session));

				try{
					(*iter)->run();
				}catch(...){
					async_erase(session.get());
					throw;
				}
			}, std::allocator< void >());
	}

	void http_sessions::async_erase(http_session* session){
		strand_.post(
			[this, lock = async_lock(async_calls_), session]{
				auto iter = set_.find(session);
				if(iter == set_.end()){
					throw std::logic_error("session doesn't exist");
				}
				set_.erase(iter);
			}, std::allocator< void >());
	}


	void http_sessions::shutdown()noexcept{
		strand_.post(
			[
				this,
				lock = async_lock(async_calls_)
			]()mutable{
				shutdown_ = true;
				for(auto& session: set_){
					session->async_erase();
				}
			}, std::allocator< void >());
	}

	bool http_sessions::is_shutdown()noexcept{
		return shutdown_;
	}

	void http_sessions::block()noexcept{
		// As long as async calls are pending
		while(async_calls_ > 0){
			// Request the server to run a handler async
			if(server_.poll_one() == 0){
				// If no handler was waiting, the pending one must
				// currently run in another thread
				std::this_thread::yield();
			}
		}
	}

}
