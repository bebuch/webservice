//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__ws_service_base__hpp_INCLUDED_
#define _webservice__ws_service_base__hpp_INCLUDED_

#include "shared_const_buffer.hpp"
#include "ws_service_interface.hpp"
#include "ws_session_settings.hpp"
#include "ws_session_container.hpp"
#include "ws_server_session.hpp"
#include "server.hpp"

#include <string>
#include <set>


namespace webservice{


	/// \brief Base for any server websocket service that handels standing
	///        sessions
	///
	/// If you derive from this class you have to override on_make calling
	/// async_make with additional ValueArgs that are used as arguments for
	/// the constructor of Value.
	template < typename Value >
	class ws_service_base
		: public ws_service_interface
		, public ws_session_settings{
	public:
		/// \brief Constructor
		ws_service_base() = default;

		/// \brief Destructor
		~ws_service_base()override{
			block();
		}


		ws_service_base(ws_service_base const&) = delete;

		ws_service_base& operator=(ws_service_base const&) = delete;


		/// \brief Send a text message to session
		void send_text(
			ws_identifier identifier,
			shared_const_buffer buffer
		){
			if(!impl_){
				throw std::logic_error(
					"called send_text() before server was set");
			}

			impl_->strand_.dispatch(
				[
					this,
					lock = impl_->locker_.make_lock("ws_session_container::send_text"),
					identifier,
					buffer = std::move(buffer)
				]()mutable{
					if(impl_->map_.count(identifier) > 0){
						identifier.session->send(true, std::move(buffer));
					}
				}, std::allocator< void >());
		}

		/// \brief Send a text message to all session
		void send_text(shared_const_buffer buffer){
			send_text_if([](Value&)noexcept{
					return true;
				}, buffer);
		}

		/// \brief Send a text message to all sessions for which fn(value)
		///        returns true
		///
		/// The value in fn(value) is the data linked to the session.
		template < typename UnaryFunction >
		void send_text_if(
			UnaryFunction fn,
			shared_const_buffer buffer
		){
			if(!impl_){
				throw std::logic_error(
					"called send_text_if() before server was set");
			}

			impl_->strand_.dispatch(
				[
					this,
					lock = impl_->locker_.make_lock("ws_session_container::send_text_if"),
					fn = std::move(fn),
					buffer = std::move(buffer)
				]()mutable{
					for(auto& session: impl_->map_){
						ws_identifier identifier(strip_const(session.first));
						try{
							if(fn(session.second)){
								identifier.session->send(true, buffer);
							}
						}catch(...){
							on_exception(identifier, std::current_exception());
						}
					}
				}, std::allocator< void >());
		}


		/// \brief Send a binary message to session
		void send_binary(
			ws_identifier identifier,
			shared_const_buffer buffer
		){
			if(!impl_){
				throw std::logic_error(
					"called send_binary() before server was set");
			}

			impl_->strand_.dispatch(
				[
					this,
					lock = impl_->locker_.make_lock("ws_session_container::send_binary"),
					identifier,
					buffer = std::move(buffer)
				]()mutable{
					if(impl_->map_.count(identifier) > 0){
						identifier.session->send(false, std::move(buffer));
					}
				}, std::allocator< void >());
		}

		/// \brief Send a binary message to all session
		void send_binary(shared_const_buffer buffer){
			send_binary_if([](Value&)noexcept{
					return true;
				}, buffer);
		}

		/// \brief Send a binary message to all sessions for which fn(value)
		///        returns true
		///
		/// The value in fn(value) is the data linked to the session.
		template < typename UnaryFunction >
		void send_binary_if(
			UnaryFunction fn,
			shared_const_buffer buffer
		){
			if(!impl_){
				throw std::logic_error(
					"called send_binary_if() before server was set");
			}

			impl_->strand_.dispatch(
				[
					this,
					lock = impl_->locker_.make_lock("ws_session_container::send_binary_if"),
					fn = std::move(fn),
					buffer = std::move(buffer)
				]()mutable{
					for(auto& session: impl_->map_){
						ws_identifier identifier(strip_const(session.first));
						try{
							if(fn(session.second)){
								identifier.session->send(false, buffer);
							}
						}catch(...){
							on_exception(identifier, std::current_exception());
						}
					}
				}, std::allocator< void >());
		}


		/// \brief Shutdown session
		void close(
			ws_identifier identifier,
			boost::beast::websocket::close_reason reason
		){
			if(!impl_){
				throw std::logic_error("called close() before server was set");
			}

			impl_->strand_.dispatch(
				[
					this,
					lock = impl_->locker_.make_lock("ws_session_container::close"),
					identifier,
					reason = std::move(reason)
				]()mutable{
					if(impl_->map_.count(identifier) > 0){
						identifier.session->close(reason);
					}

				}, std::allocator< void >());
		}

		/// \brief Shutdown all sessions
		void close(boost::beast::websocket::close_reason reason){
			close_if([](Value&)noexcept{
					return true;
				}, reason);
		}

		/// \brief Send a close to all sessions for which fn(value)
		///        returns true
		///
		/// The value in fn(value) is the data linked to the session.
		template < typename UnaryFunction >
		void close_if(
			UnaryFunction fn,
			boost::beast::websocket::close_reason reason
		){
			if(!impl_){
				throw std::logic_error(
					"called close_if() before server was set");
			}

			impl_->strand_.dispatch(
				[
					this,
					lock = impl_->locker_.make_lock("ws_session_container::close_if"),
					fn = std::move(fn),
					reason = std::move(reason)
				]()mutable{
					for(auto& session: impl_->map_){
						ws_identifier identifier(strip_const(session.first));
						try{
							if(fn(session.second)){
								identifier.session->close(reason);
							}
						}catch(...){
							on_exception(identifier, std::current_exception());
						}
					}
				}, std::allocator< void >());
		}


		/// \brief true if server is shutting down
		bool is_shutdown()noexcept{
			assert(impl_ != nullptr);

			return !impl_->run_lock_.is_locked();
		}

		/// \brief Poll server tasks until shutdown was called and the last
		///        async task finished
		void block()noexcept{
			if(!impl_) return;

			server()->poll_while([this]()noexcept{
				return impl_->locker_.count() > 0;
			});
		}


	protected:
		/// \brief Create the implementation
		///
		/// \attention: If you override on_server(), call this from your
		///             overriding function.
		void on_server()override{
			impl_ = std::make_unique< impl >(*server());
		}

		/// \brief Accept no new sessions, send close to all session
		///
		/// \attention: If you override on_shutdown(), call this from your
		///             overriding function.
		void on_shutdown()noexcept override{
			auto lock = std::move(impl_->run_lock_);
			if(lock.is_locked()){
				impl_->shutdown_lock_ = std::move(lock);

				impl_->strand_.defer(
					[this, lock = impl_->locker_.make_lock("ws_session_container::shutdown")]{
						lock.enter();

						if(impl_->map_.empty()){
							impl_->shutdown_lock_.unlock();
						}else{
							for(auto& session: impl_->map_){
								strip_const(session.first).close("shutdown");
							}
						}
					}, std::allocator< void >());
			}
		}

		/// \brief Erase the session from map_ async
		///
		/// \attention: If you override on_shutdown(), call this from your
		///             overriding function.
		void on_erase(ws_identifier identifier)noexcept override{
			assert(impl_ != nullptr);

			impl_->strand_.dispatch(
				[this, lock = impl_->locker_.make_lock("ws_session_container::on_erase"), identifier]{
					lock.enter();

					auto iter = impl_->map_.find(identifier);
					if(iter == impl_->map_.end()){
						throw std::logic_error("session doesn't exist");
					}
					impl_->map_.erase(iter);

					if(impl_->map_.empty() && is_shutdown()){
						impl_->shutdown_lock_.unlock();
					}
				}, std::allocator< void >());
		}


		/// \brief Create a new session async
		///
		/// Call this function from your on_make() overriding function to
		/// create a new websocket server session.
		template < typename ... ValueArgs >
		void async_make(
			boost::asio::ip::tcp::socket&& socket,
			http_request&& req,
			ValueArgs&& ... args
		){
			assert(server() != nullptr);

			impl_->strand_.dispatch(
				[
					this,
					lock = impl_->locker_.make_lock("ws_session_container::async_make"),
					socket = std::move(socket),
					req = std::move(req),
					args = std::make_tuple(static_cast< ValueArgs&& >(args) ...)
				](auto&& ... args)mutable{
					lock.enter();

					if(is_shutdown()){
						throw std::logic_error(
							"emplace in ws_session_container while shutdown");
					}

					ws_stream ws(std::move(socket));
					ws.read_message_max(max_read_message_size());

					auto iter = impl_->map_.emplace(std::piecewise_construct,
						std::forward_as_tuple(std::move(ws), *this,
							ping_time()), std::move(args));

					ws_identifier identifier(strip_const(iter.first->first));
					try{
						identifier.session->do_accept(std::move(req));
					}catch(...){
						on_erase(identifier);
						throw;
					}
				}, std::allocator< void >());
		}


	private:
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

		/// \brief Remove const from session
		///
		/// Map keys are always const, but we need non const session objects.
		/// The const can be safely removed because map order is done by
		/// address and the address can not change even for a non-const object.
		static constexpr ws_server_session& strip_const(
			ws_server_session const& session
		)noexcept{
			return const_cast< ws_server_session& >(session);
		}

		/// \brief implementation data after the server was set
		struct impl{
			impl(class server& server)
				: locker_([]()noexcept{})
				, run_lock_(locker_.make_first_lock("ws_session_container::ws_session_container"))
				, strand_(server.get_executor()) {}

			async_locker locker_;
			async_locker::lock run_lock_;
			async_locker::lock shutdown_lock_;
			ws_strand strand_;
			std::map< ws_server_session, Value, less > map_;
		};


		/// \brief Pointer to implementation
		std::unique_ptr< impl > impl_;
	};


}


#endif
