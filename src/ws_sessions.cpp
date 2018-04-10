//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include "ws_sessions.hpp"
#include "ws_session.hpp"

#include <webservice/server.hpp>

#include <thread>


namespace webservice{


	ws_sessions::ws_sessions(class server& server)
		: server_(server)
		, strand_(server_.get_executor()) {}


	class server* ws_sessions::server()const noexcept{
		return &server_;
	}


	void ws_sessions::async_emplace(
		http_request&& req,
		ws_stream&& ws,
		ws_handler_base& service,
		std::chrono::milliseconds ping_time
	){
		strand_.post(
			[
				this,
				lock = async_lock(async_calls_),
				req = std::move(req),
				ws = std::move(ws),
				&service,
				ping_time
			]()mutable{
				if(shutdown_){
					throw std::logic_error(
						"emplace in ws_sessions while shutdown");
				}

				auto session = std::make_unique< ws_server_session >(
					std::move(ws), service, ping_time);

				set_.insert(set_.end(), std::move(session));

				try{
					session->do_accept(std::move(req));
				}catch(...){
					async_erase(session.get());
					throw;
				}
			}, std::allocator< void >());
	}

	void ws_sessions::async_erase(ws_server_session* session){
		strand_.post(
			[this, lock = async_lock(async_calls_), session]{
				auto iter = set_.find(session);
				if(iter == set_.end()){
					throw std::logic_error("session doesn't exist");
				}
				set_.erase(iter);
			}, std::allocator< void >());
	}

	void ws_sessions::shutdown()noexcept{
		async_call([this](set const& sessions){
				shutdown_ = true;
				for(auto& session: sessions){
					session->async_erase();
				}
			});
	}

	void ws_sessions::block()noexcept{
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
