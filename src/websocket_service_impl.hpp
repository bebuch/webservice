//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__websocket_service_impl__hpp_INCLUDED_
#define _webservice__websocket_service_impl__hpp_INCLUDED_

#include <webservice/websocket_service.hpp>

#include "websocket_session.hpp"

#include <shared_mutex>


namespace webservice{


	/// \brief Implementation of websocket_service
	class websocket_service_impl{
	public:
		/// \brief Constructor
		websocket_service_impl(websocket_service& self):
			self_(self) {}

		/// \brief Called with a unique identifier when a sessions starts
		void on_open(websocket_session* const session){
			std::unique_lock lock(mutex_);
			sessions_.emplace(session);
			lock.unlock();

			self_.on_open(reinterpret_cast< std::uintptr_t >(session));
		}

		/// \brief Called with a unique identifier when a sessions ends
		void on_close(websocket_session* const session){
			self_.on_close(reinterpret_cast< std::uintptr_t >(session));

			std::unique_lock lock(mutex_);
			sessions_.erase(session);
		}

		/// \brief Called when a session received a text message
		void on_text(
			websocket_session* const session,
			boost::beast::multi_buffer& buffer
		){
			self_.on_text(
				reinterpret_cast< std::uintptr_t >(session),
				buffer);
		}

		/// \brief Called when a session received a binary message
		void on_binary(
			websocket_session* const session,
			boost::beast::multi_buffer& buffer
		){
			self_.on_binary(
				reinterpret_cast< std::uintptr_t >(session),
				buffer);
		}


		/// \brief Send a message to all sessions
		template < typename Data >
		void send(std::shared_ptr< Data > const& data){
			std::shared_lock lock(mutex_);
			for(auto const& session: sessions_){
				send(session, data);
			}
		}

		/// \brief Send a message to session by identifier
		template < typename Data >
		void send(
			std::set< std::uintptr_t > const& identifiers,
			std::shared_ptr< Data > const& data
		){
			std::shared_lock lock(mutex_);
			auto iter = identifiers.begin();
			auto const end = identifiers.end();
			for(auto const session: sessions_){
				auto const identifier =
					reinterpret_cast< std::uintptr_t const >(session);

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
			std::shared_ptr< Data >&& data
		){
			std::shared_lock lock(mutex_);
			auto const iter = sessions_.find(
				reinterpret_cast< websocket_session* const >(identifier));
			if(iter == sessions_.end()){
				return;
			}

			send(*iter, std::move(data));
		}


	private:
		/// \brief Send a message to session
		template < typename Data >
		void send(
			websocket_session* const session,
			std::shared_ptr< Data > data
		){
			session->send(std::move(data));
		}


		/// \brief Reference to the actual object
		websocket_service& self_;

		/// \brief Protect sessions_
		std::shared_mutex mutex_;

		/// \brief List of sessions registered by open, erased by close
		std::set< websocket_session* > sessions_;
	};


}


#endif
