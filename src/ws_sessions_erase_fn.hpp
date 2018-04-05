//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__ws_sessions_erase_fn__hpp_INCLUDED_
#define _webservice__ws_sessions_erase_fn__hpp_INCLUDED_

#include <list>


namespace webservice{


	class ws_server_session;

	class ws_sessions_erase_fn{
	public:
		using iterator = typename std::list< ws_server_session >::iterator;

		ws_sessions_erase_fn()noexcept = default;

		ws_sessions_erase_fn(
			class ws_sessions* ws_sessions,
			iterator iter
		)noexcept;

		ws_sessions_erase_fn(ws_sessions_erase_fn&& other)noexcept;

		ws_sessions_erase_fn& operator=(ws_sessions_erase_fn&& other)noexcept;


		void operator()();


	private:
		class ws_sessions* ws_sessions_{nullptr};
		iterator iter_;
	};


}


#endif
