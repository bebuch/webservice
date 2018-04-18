//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__ws_session_container__hpp_INCLUDED_
#define _webservice__ws_session_container__hpp_INCLUDED_

#include "ws_identifier.hpp"
#include "async_lock.hpp"

#include <boost/beast/websocket.hpp>

#include <boost/asio/strand.hpp>

#include <map>
#include <tuple>


namespace webservice{


	using ws_stream
		= boost::beast::websocket::stream< boost::asio::ip::tcp::socket >;

	using ws_strand
		= boost::asio::strand< boost::asio::io_context::executor_type >;

	using http_request
		= boost::beast::http::request< boost::beast::http::string_body >;


	class ws_server_session;

	template < typename Value >
	class ws_service_base;


	template < typename Value >
	class ws_session_container{
		struct less{
			using is_transparent = void;

			bool operator()(
				ws_server_session const& l,
				ws_server_session const& r
			)const noexcept{
				return &l < &r;
			}

			bool operator()(
				ws_server_session* l,
				ws_server_session const& r
			)const noexcept{
				return l < &r;
			}

			bool operator()(
				ws_server_session const& l,
				ws_server_session* r
			)const noexcept{
				return &l < r;
			}

			bool operator()(
				ws_identifier l,
				ws_server_session const& r
			)const noexcept{
				return l.session < &r;
			}

			bool operator()(
				ws_server_session const& l,
				ws_identifier r
			)const noexcept{
				return &l < r.session;
			}
		};

	public:
		using map = std::map< ws_server_session, Value, less >;


		ws_session_container()
			: locker_([]()noexcept{})
			, run_lock_(locker_.make_first_lock("ws_session_container::ws_session_container"))
			, strand_(server_.get_executor()) {}

		ws_session_container(ws_session_container const&) = delete;


		ws_session_container& operator=(ws_session_container const&) = delete;


		template < typename ... ValueArgs >
		void async_make(
			http_request&& req,
			ws_stream&& ws,
			ws_handler_base& service,
			std::chrono::milliseconds ping_time,
			ValueArgs&& ... args
		){
			strand_.dispatch(
				[
					this,
					lock = locker_.make_lock("ws_session_container::async_make"),
					req = std::move(req),
					ws = std::move(ws),
					&service,
					ping_time,
					args = std::make_tuple(static_cast< Args&& >(args) ...)
				](auto&& ... args)mutable{
					lock.enter();

					if(is_shutdown()){
						throw std::logic_error(
							"emplace in ws_session_container while shutdown");
					}

					auto iter = map_.emplace(std::piecewise_construct,
						std::forward_as_tuple(std::move(ws), service,
							ping_time), std::move(args));

					try{
						(*iter)->do_accept(std::move(req));
					}catch(...){
						async_erase(session.get());
						throw;
					}
				}, std::allocator< void >());
		}

		void async_erase(ws_server_session* session){
			strand_.dispatch(
				[this, lock = locker_.make_lock("ws_session_container::async_erase"), session]{
					lock.enter();

					auto iter = map_.find(session);
					if(iter == map_.end()){
						throw std::logic_error("session doesn't exist");
					}
					map_.erase(iter);

					if(map_.empty() && is_shutdown()){
						shutdown_lock_.unlock();
					}
				}, std::allocator< void >());
		}


		template < typename Fn >
		void async_call(Fn&& fn){
			strand_.dispatch(
				[
					this,
					lock = locker_.make_lock("ws_session_container::async_call"),
					fn = static_cast< Fn&& >(fn)
				]()mutable{
					lock.enter();

					fn(map_);
				}, std::allocator< void >());
		}


		/// \brief Accept no new sessions, send close to all session
		void shutdown()noexcept{
			auto lock = std::move(run_lock_);
			if(lock.is_locked()){
				shutdown_lock_ = std::move(lock);

				strand_.defer(
					[this, lock = locker_.make_lock("ws_session_container::shutdown")]{
						lock.enter();

						if(map_.empty()){
							shutdown_lock_.unlock();
						}else{
							for(auto& session: map_){
								session->send("shutdown");
							}
						}
					}, std::allocator< void >());
			}
		}

		/// \brief true if server is shutting down
		bool is_shutdown()noexcept{
			return !run_lock_.is_locked();
		}

		/// \brief Poll server tasks until shutdown was called and the last
		///        async task finished
		void block(server& server)noexcept{
			server.poll_while([this]()noexcept{
				return locker_.count() > 0;
			});
		}


	private:
		async_locker locker_;
		async_locker::lock run_lock_;
		async_locker::lock shutdown_lock_;
		ws_strand strand_;
		map map_;
	};


}


#endif
