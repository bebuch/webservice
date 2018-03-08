//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__checked_ws_service_base_impl__hpp_INCLUDED_
#define _webservice__checked_ws_service_base_impl__hpp_INCLUDED_

#include <webservice/checked_ws_service_base.hpp>

#include "ws_session.hpp"

#include <shared_mutex>


namespace webservice{


	/// \brief Implementation of checked_ws_service_base
	class checked_ws_service_base_impl{
	public:
		/// \brief Constructor
		checked_ws_service_base_impl(checked_ws_service_base& self):
			self_(self) {}

		/// \brief Called with a unique identifier when a sessions starts
		void on_open(
			ws_server_session* const session,
			std::string const& resource
		){
			std::unique_lock< std::shared_timed_mutex > lock(mutex_);
			sessions_.emplace(session);
			lock.unlock();

			self_.on_open(
				reinterpret_cast< std::uintptr_t >(session), resource);
		}

		/// \brief Called with a unique identifier when a sessions ends
		void on_close(
			ws_server_session* const session,
			std::string const& resource
		){
			self_.on_close(
				reinterpret_cast< std::uintptr_t >(session), resource);

			std::unique_lock< std::shared_timed_mutex > lock(mutex_);
			sessions_.erase(session);
		}

		/// \brief Called when a session received a text message
		void on_text(
			ws_server_session* const session,
			std::string const& resource,
			boost::beast::multi_buffer const& buffer
		){
			self_.on_text(
				reinterpret_cast< std::uintptr_t >(session),
				resource,
				buffer);
		}

		/// \brief Called when a session received a binary message
		void on_binary(
			ws_server_session* const session,
			std::string const& resource,
			boost::beast::multi_buffer const& buffer
		){
			self_.on_binary(
				reinterpret_cast< std::uintptr_t >(session),
				resource,
				buffer);
		}

		/// \brief Called when an error occured
		void on_error(
			ws_server_session* const session,
			std::string const& resource,
			ws_service_location location,
			boost::system::error_code ec
		){
			self_.on_error(
				reinterpret_cast< std::uintptr_t >(session),
				resource,
				location,
				ec);
		}

		/// \brief Called when an exception was thrown
		void on_exception(
			ws_server_session* const session,
			std::string const& resource,
			std::exception_ptr error
		)noexcept{
			self_.on_exception(
				reinterpret_cast< std::uintptr_t >(session),
				resource,
				error);
		}


		/// \brief Send a message to all sessions
		template < typename Data >
		void send(Data const& data){
			std::shared_lock< std::shared_timed_mutex > lock(mutex_);
			for(auto const& session: sessions_){
				send(session, data);
			}
		}

		/// \brief Send a message to session by identifier
		template < typename Data >
		void send(
			std::set< std::uintptr_t > const& identifiers,
			Data const& data
		){
			std::shared_lock< std::shared_timed_mutex > lock(mutex_);
			auto iter = identifiers.begin();
			auto const end = identifiers.end();
			for(auto const session: sessions_){
				auto const identifier =
					reinterpret_cast< std::uintptr_t >(session);

				while(iter != end && *iter != identifier){
					++iter;
				}

				if(iter == end){
					break;
				}

				send(session, data);
				++iter;
			}
		}

		/// \brief Send a message to session by identifiers
		template < typename Data >
		void send(
			std::uintptr_t const identifier,
			Data&& data
		){
			std::shared_lock< std::shared_timed_mutex > lock(mutex_);
			auto const iter = sessions_.find(reinterpret_cast<
				ws_server_session* >(identifier));
			if(iter == sessions_.end()){
				return;
			}

			send(*iter, static_cast< Data&& >(data));
		}


	private:
		/// \brief Send a message to session
		template < typename Data >
		void send(
			ws_server_session* const session,
			Data&& data
		){
			session->send(static_cast< Data&& >(data));
		}


		/// \brief Reference to the actual object
		checked_ws_service_base& self_;

		/// \brief Protect sessions_
		///
		/// We don't need the timed-part, but C++14 has no non-timed
		/// shared_mutex.
		std::shared_timed_mutex mutex_;

		/// \brief List of sessions registered by open, erased by close
		std::set< ws_server_session* > sessions_;
	};


}


#endif
