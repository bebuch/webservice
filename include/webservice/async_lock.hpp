//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__async_lock__hpp_INCLUDED_
#define _webservice__async_lock__hpp_INCLUDED_

#include <atomic>
#include <utility>
#include <iostream>
#include <mutex>


namespace webservice{


	class async_lock{
	public:
		static std::mutex mutex;

		async_lock(std::atomic< std::size_t >& lock_count, char const* op)
			: lock_count_(&lock_count)
			, op_(op)
		{
			count_ = ++*lock_count_;
			std::lock_guard< std::mutex > lock(mutex);
			std::cout << lock_count_ << ">" << op_ << (count_) << std::endl;
		}

		async_lock(async_lock const&) = delete;

		async_lock(async_lock&& other)
			: lock_count_(std::exchange(other.lock_count_, nullptr))
			, count_(other.count_)
			, op_(other.op_) {}

		~async_lock(){
			if(lock_count_){
				--*lock_count_;
				std::lock_guard< std::mutex > lock(mutex);
				std::cout << lock_count_ << "<" << op_ << (count_) << std::endl;
			}
		}

		async_lock& operator=(async_lock const&) = delete;


	private:
		std::atomic< std::size_t >* lock_count_;
		std::size_t count_;
		char const* op_;
	};


}


#endif
