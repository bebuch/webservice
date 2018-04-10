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


	void ws_sessions::set_server(class server& server){
		server_ = &server;
	}

	class server* ws_sessions::server()const noexcept{
		return server_;
	}


	bool ws_sessions::is_empty()const{
		std::shared_lock< std::shared_timed_mutex > lock(mutex_);
		return list_.empty();
	}

	std::size_t ws_sessions::size()const{
		std::shared_lock< std::shared_timed_mutex > lock(mutex_);
		return list_.size();
	}

	void ws_sessions::emplace(
		http_request&& req,
		ws_stream&& ws,
		ws_handler_base& service,
		std::chrono::milliseconds ping_time
	){
		std::unique_lock< std::shared_timed_mutex > lock(mutex_);
		if(shutdown_){
			throw std::logic_error("emplace in ws_sessions while shutdown");
		}

		auto iter = list_.emplace(list_.end(),
			std::move(ws), service, ping_time);
		set_.insert(set_.end(), ws_identifier(&*iter));
		iter->set_erase_fn(ws_sessions_erase_fn(this, iter));
		iter->do_accept(std::move(req));
	}


	void ws_sessions::erase(iterator iter){
		std::unique_lock< std::shared_timed_mutex > lock(mutex_);
		set_.erase(ws_identifier(&*iter));
		list_.erase(iter);
	}

	void ws_sessions::shutdown()noexcept{
		std::shared_lock< std::shared_timed_mutex > lock(mutex_);
		shutdown_ = true;

		for(auto& session: list_){
			session.async_erase();
		}
	}

	void ws_sessions::block()noexcept{
		// Block until last element has been removed from list
		while(!is_empty()){
			assert(server() != nullptr);
			if(server()->poll_one() == 0){
				std::this_thread::yield();
			}
		}

		// Block until last erase has been completed
		std::unique_lock< std::shared_timed_mutex > lock(mutex_);
	}


}
