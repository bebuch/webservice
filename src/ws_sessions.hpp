//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__ws_sessions__hpp_INCLUDED_
#define _webservice__ws_sessions__hpp_INCLUDED_

#include "ws_sessions_erase_fn.hpp"

#include <webservice/ws_identifier.hpp>

#include <boost/beast/websocket.hpp>

#include <set>
#include <shared_mutex>


namespace webservice{


	using ws_stream
		= boost::beast::websocket::stream< boost::asio::ip::tcp::socket >;

	using http_request
		= boost::beast::http::request< boost::beast::http::string_body >;

	class ws_server_session;
	class ws_handler_base;

	class ws_sessions{
	public:
		using iterator = typename std::list< ws_server_session >::iterator;
		using const_iterator
			= typename std::list< ws_server_session >::const_iterator;

		ws_sessions() = default;

		ws_sessions(ws_sessions const&) = default;


		void set_server(class server& server);

		class server* server()const noexcept;


		bool is_empty()const;

		std::size_t size()const;

		void emplace(
			http_request&& req,
			ws_stream&& ws,
			ws_handler_base& service,
			std::chrono::milliseconds ping_time
		);


		template < typename Fn >
		auto shared_call(Fn&& fn)const{
			std::shared_lock< std::shared_timed_mutex > lock(mutex_);
			return fn(set_);
		}


		void erase(iterator iter);

		void shutdown()noexcept;

		void block()noexcept;


	private:
		bool shutdown_{false};
		std::shared_timed_mutex mutable mutex_;
		std::list< ws_server_session > list_;
		std::set< ws_identifier > set_;
		class server* server_{nullptr};
	};


}


#endif
