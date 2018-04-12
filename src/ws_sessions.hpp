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

#include <webservice/ws_identifier.hpp>
#include <webservice/async_lock.hpp>

#include <boost/beast/websocket.hpp>

#include <boost/asio/strand.hpp>

#include <set>
#include <memory>


namespace webservice{


	using ws_stream
		= boost::beast::websocket::stream< boost::asio::ip::tcp::socket >;

	using ws_strand
		= boost::asio::strand< boost::asio::io_context::executor_type >;

	using http_request
		= boost::beast::http::request< boost::beast::http::string_body >;


	class ws_server_session;
	class ws_handler_base;


	class ws_sessions{
		struct less{
			using is_transparent = void;

			bool operator()(
				std::unique_ptr< ws_server_session > const& l,
				std::unique_ptr< ws_server_session > const& r
			)const noexcept{
				return l.get() < r.get();
			}

			bool operator()(
				ws_server_session* l,
				std::unique_ptr< ws_server_session > const& r
			)const noexcept{
				return l < r.get();
			}

			bool operator()(
				std::unique_ptr< ws_server_session > const& l,
				ws_server_session* r
			)const noexcept{
				return l.get() < r;
			}

			bool operator()(
				ws_identifier l,
				std::unique_ptr< ws_server_session > const& r
			)const noexcept{
				return l.session < r.get();
			}

			bool operator()(
				std::unique_ptr< ws_server_session > const& l,
				ws_identifier r
			)const noexcept{
				return l.get() < r.session;
			}
		};

	public:
		using set = std::set< std::unique_ptr< ws_server_session >, less >;


		ws_sessions(class server& server);

		ws_sessions(ws_sessions const&) = delete;


		ws_sessions& operator=(ws_sessions const&) = delete;


		class server* server()const noexcept;


		void async_emplace(
			http_request&& req,
			ws_stream&& ws,
			ws_handler_base& service,
			std::chrono::milliseconds ping_time
		);

		void async_erase(ws_server_session* session);


		template < typename Fn >
		void async_call(Fn&& fn){
			strand_.post(
				[
					this,
					lock = async_lock(async_calls_, "ws_sessions::async_call"),
					fn = static_cast< Fn&& >(fn)
				]()mutable{
					fn(set_);
				}, std::allocator< void >());
		}


		void shutdown()noexcept;

		/// \brief true if server is shutting down
		bool is_shutdown()noexcept;

		void block()noexcept;


	private:
		class server& server_;
		bool shutdown_{false};
		std::atomic< std::size_t > async_calls_{0};
		ws_strand strand_;
		set set_;
	};


}


#endif
