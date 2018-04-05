//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__ws_identifier__hpp_INCLUDED_
#define _webservice__ws_identifier__hpp_INCLUDED_

#include <ostream>


namespace webservice{


	class ws_server_session;
	class ws_handler_base;

	class ws_identifier{
	private:
		explicit constexpr ws_identifier(ws_server_session* session)noexcept
			: session(session) {}

		ws_server_session* session;

		template < typename CharT, typename Traits >
		friend std::basic_ostream< CharT, Traits >& operator<<(
			std::basic_ostream< CharT, Traits >& os,
			ws_identifier identifier
		){
			return os << identifier.session;
		}


		friend constexpr bool operator==(
			ws_identifier l,
			ws_identifier r
		)noexcept{
			return l.session == r.session;
		}

		friend constexpr bool operator!=(
			ws_identifier l,
			ws_identifier r
		)noexcept{
			return l.session != r.session;
		}

		friend constexpr bool operator<(
			ws_identifier l,
			ws_identifier r
		)noexcept{
			return l.session < r.session;
		}

		friend constexpr bool operator>(
			ws_identifier l,
			ws_identifier r
		)noexcept{
			return l.session > r.session;
		}

		friend constexpr bool operator<=(
			ws_identifier l,
			ws_identifier r
		)noexcept{
			return l.session <= r.session;
		}

		friend constexpr bool operator>=(
			ws_identifier l,
			ws_identifier r
		)noexcept{
			return l.session >= r.session;
		}


		friend class ws_handler_base;
		friend class ws_server_session;
		friend class ws_sessions;
	};


}


#endif
