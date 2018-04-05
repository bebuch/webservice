//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__ws_sessions__hpp_INCLUDED_
#define _webservice__ws_sessions__hpp_INCLUDED_

#include "ws_sessions_erase_fn.hpp"

#include <webservice/ws_identifier.hpp>

#include <set>
#include <shared_mutex>


namespace webservice{


	class ws_server_session;

	class ws_sessions{
	public:
		using iterator = typename std::list< ws_server_session >::iterator;
		using const_iterator
			= typename std::list< ws_server_session >::const_iterator;

		ws_sessions() = default;

		ws_sessions(ws_sessions const&) = default;

		~ws_sessions();


		void set_server(class server& server);

		class server* server()const noexcept;


		bool is_empty()const;

		std::size_t size()const;

		template < typename ... Ts >
		iterator emplace(Ts&& ... vs){
			std::unique_lock< std::shared_timed_mutex > lock(mutex_);
			if(shutdown_){
				throw std::logic_error("emplace in ws_sessions while shutdown");
			}

			auto iter = list_.emplace(list_.end(), static_cast< Ts&& >(vs) ...);
			set_.insert(set_.end(), ws_identifier(&*iter));
			iter->set_erase_fn(ws_sessions_erase_fn(this, iter));
			return iter;
		}

		template < typename Fn >
		auto unique_call(Fn&& fn){
			std::unique_lock< std::shared_timed_mutex > lock(mutex_);
			return fn(set_);
		}

		template < typename Fn >
		auto shared_call(Fn&& fn)const{
			std::shared_lock< std::shared_timed_mutex > lock(mutex_);
			return fn(set_);
		}


		void erase(iterator iter);


	private:
		bool shutdown_{false};
		std::shared_timed_mutex mutable mutex_;
		std::list< ws_server_session > list_;
		std::set< ws_identifier > set_;
		class server* server_{nullptr};
	};


}


#endif
