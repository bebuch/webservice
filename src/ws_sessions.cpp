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
