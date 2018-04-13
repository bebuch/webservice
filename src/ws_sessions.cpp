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
		, locker_([]()noexcept{})
		, run_lock_(locker_.make_first_lock("ws_sessions::ws_sessions"))
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
		strand_.dispatch(
			[
				this,
				lock = locker_.make_lock("ws_sessions::async_emplace"),
				req = std::move(req),
				ws = std::move(ws),
				&service,
				ping_time
			]()mutable{
				lock.enter();

				if(is_shutdown()){
					throw std::logic_error(
						"emplace in ws_sessions while shutdown");
				}

				auto session = std::make_unique< ws_server_session >(
					std::move(ws), service, ping_time);

				auto iter = set_.insert(set_.end(), std::move(session));

				try{
					(*iter)->do_accept(std::move(req));
				}catch(...){
					async_erase(session.get());
					throw;
				}
			}, std::allocator< void >());
	}

	void ws_sessions::async_erase(ws_server_session* session){
		strand_.dispatch(
			[this, lock = locker_.make_lock("ws_sessions::async_erase"), session]{
				lock.enter();

				auto iter = set_.find(session);
				if(iter == set_.end()){
					throw std::logic_error("session doesn't exist");
				}
				set_.erase(iter);
			}, std::allocator< void >());
	}

	void ws_sessions::shutdown()noexcept{
		strand_.defer(
			[this, lock = locker_.make_lock("ws_sessions::shutdown")]{
				lock.enter();

				for(auto& session: set_){
					session->send("shutdown");
				}
			}, std::allocator< void >());

		run_lock_.unlock();
	}

	bool ws_sessions::is_shutdown()noexcept{
		return !run_lock_.is_locked();
	}

	void ws_sessions::block()noexcept{
		server_.poll_while([this]()noexcept{ return locker_.count() > 0; });
	}


}
