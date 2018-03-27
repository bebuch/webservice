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

#include <list>
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
			sessions< T >* list,
			iterator iter
		)noexcept
			: list_(s)
			, iter_(iter) {}

		sessions_eraser(sessions_eraser&& other)noexcept
			: sessions_eraser(other.list_, other.iter_)
		{
			other.list_ = nullptr;
		}

		sessions_eraser& operator=(sessions_eraser&& other)noexcept{
			list_ = other.list_;
			iter_ = other.iter_;
			other.list_ = nullptr;
			return *this;
		}


		void operator()(){
			assert(list_);
			list_->erase(iter_);
		}

		iterator iter()noexcept{
			return iter_;
		}

	private:
		sessions< T >* list_{nullptr};
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
				server_.pull();
			}
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
			std::unique_lock< std::shared_timed_mutex > lock(mutex_);
			std::unique_lock< std::shared_timed_mutex > lock(other.mutex_);
			list_.splice(pos, std::move(other.list_));
		}

		void splice(const_iterator pos, sessions&& other, const_iterator iter){
			std::unique_lock< std::shared_timed_mutex > lock(mutex_);
			std::unique_lock< std::shared_timed_mutex > lock(other.mutex_);
			list_.splice(pos, std::move(other.list_), iter);
		}


	private:
		void erase(iterator iter){
			std::unique_lock< std::shared_timed_mutex > lock(mutex_);
			if(list_.erase(iter) < 1){
				throw does_not_exist("in sessions erase");
			}
		}

		std::atomic< bool > shutdown_{false};
		std::shared_timed_mutex mutable mutex_;
		std::list< T > list_;
	};


}


#endif
