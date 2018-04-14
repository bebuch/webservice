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
		, locker_([]()noexcept{})
		, run_lock_(locker_.make_first_lock("http_sessions::http_sessions"))
		, strand_(server_.get_executor()) {}


	class server* http_sessions::server()const noexcept{
		return &server_;
	}


	void http_sessions::async_emplace(
		boost::asio::ip::tcp::socket&& socket,
		http_request_handler& handler
	){
		strand_.dispatch(
			[
				this,
				lock = locker_.make_lock("http_sessions::async_emplace"),
				socket = std::move(socket),
				&handler
			]()mutable{
				lock.enter();

				if(is_shutdown()){
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
		strand_.dispatch(
			[this, lock = locker_.make_lock("http_sessions::async_erase"), session]{
				lock.enter();

				auto iter = set_.find(session);
				if(iter == set_.end()){
					throw std::logic_error("session doesn't exist");
				}
				set_.erase(iter);

				if(set_.empty() && is_shutdown()){
					shutdown_lock_.unlock();
				}
			}, std::allocator< void >());
	}


	void http_sessions::shutdown()noexcept{
		auto lock = std::move(run_lock_);
		if(lock.is_locked()){
			shutdown_lock_ = std::move(lock);

			strand_.defer(
				[
					this,
					lock = locker_.make_lock("http_sessions::shutdown")
				]()mutable{
					lock.enter();

					if(set_.empty()){
						shutdown_lock_.unlock();
					}else{
						for(auto& session: set_){
							session->do_close();
						}
					}
				}, std::allocator< void >());
		}
	}

	bool http_sessions::is_shutdown()noexcept{
		return !run_lock_.is_locked();
	}

	void http_sessions::block()noexcept{
		server_.poll_while([this]()noexcept{
				return locker_.count() > 0;
			});
	}


}
