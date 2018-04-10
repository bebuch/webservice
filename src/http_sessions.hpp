//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__http_sessions__hpp_INCLUDED_
#define _webservice__http_sessions__hpp_INCLUDED_

#include "http_sessions_erase_fn.hpp"

#include <list>
#include <shared_mutex>


namespace webservice{


	class http_session;


	class http_sessions{
	public:
		using iterator = typename std::list< http_session >::iterator;
		using const_iterator
			= typename std::list< http_session >::const_iterator;

		http_sessions() = default;

		http_sessions(http_sessions const&) = default;


		void set_server(class server& server);

		class server* server()const noexcept;


		bool is_empty()const;

		std::size_t size()const;

		iterator emplace(
			boost::asio::ip::tcp::socket&& socket,
			server_impl& server
		);


		void erase(iterator iter);

		void shutdown()noexcept;

		void block()noexcept;


	private:
		bool shutdown_{false};
		std::shared_timed_mutex mutable mutex_;
		std::list< http_session > list_;
		class server* server_{nullptr};
	};


}


#endif
