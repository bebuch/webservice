//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include "http_sessions_erase_fn.hpp"
#include "http_sessions.hpp"

#include <utility>
#include <cassert>


namespace webservice{


	http_sessions_erase_fn::http_sessions_erase_fn(
		class http_sessions* http_sessions,
		iterator iter
	)noexcept
		: http_sessions_(http_sessions)
		, iter_(iter) {}

	http_sessions_erase_fn::http_sessions_erase_fn(
		http_sessions_erase_fn&& other
	)noexcept
		: http_sessions_(std::exchange(other.http_sessions_, nullptr))
		, iter_(other.iter_) {}

	http_sessions_erase_fn& http_sessions_erase_fn::operator=(
		http_sessions_erase_fn&& other
	)noexcept{
		http_sessions_ = other.http_sessions_;
		iter_ = other.iter_;
		other.http_sessions_ = nullptr;
		return *this;
	}

	void http_sessions_erase_fn::operator()(){
		assert(http_sessions_);
		http_sessions_->erase(iter_);
	}


}
