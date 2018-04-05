//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include "ws_sessions_erase_fn.hpp"
#include "ws_sessions.hpp"

#include <utility>
#include <cassert>


namespace webservice{


	ws_sessions_erase_fn::ws_sessions_erase_fn(
		class ws_sessions* ws_sessions,
		iterator iter
	)noexcept
		: ws_sessions_(ws_sessions)
		, iter_(iter) {}

	ws_sessions_erase_fn::ws_sessions_erase_fn(
		ws_sessions_erase_fn&& other
	)noexcept
		: ws_sessions_(std::exchange(other.ws_sessions_, nullptr))
		, iter_(other.iter_) {}

	ws_sessions_erase_fn& ws_sessions_erase_fn::operator=(
		ws_sessions_erase_fn&& other
	)noexcept{
		ws_sessions_ = other.ws_sessions_;
		iter_ = other.iter_;
		other.ws_sessions_ = nullptr;
		return *this;
	}

	void ws_sessions_erase_fn::ws_sessions_erase_fn::operator()(){
		assert(ws_sessions_);
		ws_sessions_->erase(iter_);
	}


}
