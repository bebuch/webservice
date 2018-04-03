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


namespace webservice{


	class async_lock{
	public:
		async_lock(std::atomic< std::size_t >& lock_count)
			: lock_count_(&lock_count)
		{
			++*lock_count_;
		}

		async_lock(async_lock const&) = delete;

		async_lock(async_lock&& other)
			: lock_count_(std::exchange(other.lock_count_, nullptr)) {}

		~async_lock(){
			if(lock_count_){
				--*lock_count_;
			}
		}

		async_lock& operator=(async_lock const&) = delete;


	private:
		std::atomic< std::size_t >* lock_count_;
	};


}


#endif
