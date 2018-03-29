//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__sessions__hpp_INCLUDED_
#define _webservice__sessions__hpp_INCLUDED_

#include "server_impl.hpp"

#include <list>
#include <atomic>
#include <shared_mutex>


namespace webservice{


	struct duplicate_insert: std::logic_error{
		duplicate_insert(std::string const& message)
			: std::logic_error("duplicate insert: " + message) {}
	};

	struct does_not_exist: std::logic_error{
		does_not_exist(std::string const& message)
			: std::logic_error("does not exist: " + message) {}
	};


	template < typename T >
	class sessions;

	template < typename T >
	class sessions_eraser{
	public:
		using iterator = typename sessions< T >::iterator;

		sessions_eraser()noexcept = default;

		sessions_eraser(
			class sessions< T >* sessions,
			iterator iter
		)noexcept
			: sessions_(sessions)
			, iter_(iter) {}

		sessions_eraser(sessions_eraser&& other)noexcept
			: sessions_eraser(other.sessions_, other.iter_)
		{
			other.sessions_ = nullptr;
		}

		sessions_eraser& operator=(sessions_eraser&& other)noexcept{
			sessions_ = other.sessions_;
			iter_ = other.iter_;
			other.sessions_ = nullptr;
			return *this;
		}


		void operator()(){
			assert(sessions_);
			sessions_->erase(iter_);
		}

		iterator iter()noexcept{
			return iter_;
		}

	private:
		class sessions< T >* sessions_{nullptr};
		iterator iter_;
	};


	template < typename T >
	class sessions{
	public:
		using iterator = typename std::list< T >::iterator;
		using const_iterator = typename std::list< T >::const_iterator;

		sessions() = default;

		sessions(sessions const&) = default;

		~sessions(){
			std::shared_lock< std::shared_timed_mutex > lock(mutex_);
			shutdown_ = true;

			for(auto& session: list_){
				session.async_erase(sessions_eraser< T >());
			}
			lock.unlock();

			while(!is_empty()){
				server_->poll_one();
			}
		}


		void set_server(class server_impl* server){
			server_ = server;
		}

		class server_impl* server()const noexcept{
			return server_;
		}


		bool is_empty()const{
			std::shared_lock< std::shared_timed_mutex > lock(mutex_);
			return list_.empty();
		}

		std::size_t size()const{
			std::shared_lock< std::shared_timed_mutex > lock(mutex_);
			return list_.size();
		}

		template < typename ... Ts >
		iterator emplace(Ts&& ... vs){
			if(shutdown_){
				throw std::logic_error("emplace in sessions while shutdown");
			}else{
				std::unique_lock< std::shared_timed_mutex > lock(mutex_);
				auto pair = list_.emplace(static_cast< Ts&& >(vs) ...);
				if(!pair.second){
					throw duplicate_insert("in sessions emplace");
				}
				pair.second.set_erase_fn();
				return pair.second;
			}
		}

		template < typename Fn >
		auto call(Fn&& fn){
			std::unique_lock< std::shared_timed_mutex > lock(mutex_);
			return fn(list_);
		}

		template < typename Fn >
		auto const_call(Fn&& fn)const{
			std::shared_lock< std::shared_timed_mutex > lock(mutex_);
			return fn(list_);
		}

		void splice(const_iterator pos, sessions&& other){
			std::unique_lock< std::shared_timed_mutex > lock1(mutex_);
			std::unique_lock< std::shared_timed_mutex > lock2(other.mutex_);
			list_.splice(pos, std::move(other.list_));
		}

		void splice(const_iterator pos, sessions&& other, const_iterator iter){
			std::unique_lock< std::shared_timed_mutex > lock1(mutex_);
			std::unique_lock< std::shared_timed_mutex > lock2(other.mutex_);
			list_.splice(pos, std::move(other.list_), iter);
		}

		void erase(iterator iter){
			std::unique_lock< std::shared_timed_mutex > lock(mutex_);
			list_.erase(iter);
		}


	private:
		std::atomic< bool > shutdown_{false};
		std::shared_timed_mutex mutable mutex_;
		std::list< T > list_;
		class server_impl* server_{nullptr};
	};


}


#endif
