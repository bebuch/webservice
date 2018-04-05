//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__http_sessions_erase_fn__hpp_INCLUDED_
#define _webservice__http_sessions_erase_fn__hpp_INCLUDED_

#include <webservice/server.hpp>

#include <list>


namespace webservice{


	class http_session;

	class http_sessions_erase_fn{
	public:
		using iterator = typename std::list< http_session >::iterator;

		http_sessions_erase_fn()noexcept = default;

		http_sessions_erase_fn(
			class http_sessions* http_sessions,
			iterator iter
		)noexcept;

		http_sessions_erase_fn(http_sessions_erase_fn&& other)noexcept;

		http_sessions_erase_fn& operator=(
			http_sessions_erase_fn&& other
		)noexcept;


		void operator()();


	private:
		class http_sessions* http_sessions_{nullptr};
		iterator iter_;
	};


}


#endif
