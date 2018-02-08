//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webserver
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <webserver/websocket_service.hpp>

#include "websocket_session.hpp"

#include <shared_mutex>


namespace webserver{


	/// \brief Implementation of websocket_service
	class websocket_service_impl{
	public:
		/// \brief Constructor
		websocket_service_impl(websocket_service& self):
			self_(self) {}

		/// \brief Called with a unique identifier when a sessions starts
		void on_open(std::weak_ptr< websocket_session >&& session){
			{
				std::unique_lock lock(mutex_);
				sessions_.emplace(std::move(setting));
			}
			self_.on_open(
				reinterpret_cast< std::size_t >(session.get()));
		}

		/// \brief Called with a unique identifier when a sessions ends
		void on_close(std::weak_ptr< websocket_session > const& session){
			self_.on_close(
				reinterpret_cast< std::size_t >(session.get()));

			std::unique_lock lock(mutex_);
			sessions_.erase(setting);
		}

		/// \brief Called when a session received a text message
		void on_text(
			std::weak_ptr< websocket_session >&& session,
			std::string&& data
		){
			self_.on_text(
				reinterpret_cast< std::size_t >(session.get()),
				std::move(data));
		}

		/// \brief Called when a session received a binary message
		void on_binary(
			std::weak_ptr< websocket_session >&& session,
			std::vector< std::uint8_t >&& data
		){
			self_.on_binary(
				reinterpret_cast< std::size_t >(session.get()),
				std::move(data));
		}


		/// \brief Send a text message to all sessions
		void send_text(std::string&& data){
			std::shared_lock lock(mutex_);
				auto locked_session = session.lock();
				if(locked_session){
					send_text(locked_session, data);
				}
		}

		/// \brief Send a text message to session with identifier
		void send_text(std::size_t identifier, std::string&& data){
			std::shared_lock lock(mutex_);
			auto const iter = sessions_.find(identifier);
			if(iter == sessions_.end()){
				return;
			}

			auto const session = *iter;
			lock.unlock();

			auto const locked_session = session.lock();
			if(locked_session){
				send_text(locked_session, std::move(data));
			}
		}

		/// \brief Send a binary message to all sessions
		void send_binary(std::vector< std::uint8_t >&& data){
			std::shared_lock lock(mutex_);
			for(auto& session: sessions_){
				auto locked_session = session.lock();
				if(locked_session){
					send_binary(locked_session, data);
				}
			}
		}

		/// \brief Send a binary message to session with identifier
		void send_binary(
			std::size_t identifier,
			std::vector< std::uint8_t >&& data
		){
			std::shared_lock lock(mutex_);
			auto const iter = sessions_.find(identifier);
			if(iter == sessions_.end()){
				return;
			}

			auto const session = *iter;
			lock.unlock();

			auto const locked_session = session.lock();
			if(locked_session){
				send_binary(locked_session, std::move(data));
			}
		}


	private:
		/// \brief Send a text message to session
		void send_text(
			std::shared_ptr< websocket_session > const& session,
			std::string data
		){
			session->send_text(std::move(data));
		}

		/// \brief Send a binary message to session
		void send_binary(
			std::shared_ptr< websocket_session > const& session,
			std::vector< std::uint8_t > data
		){
			session->send_binary(std::move(data));
		}


		/// \brief Reference to the actual object
		websocket_service& self_;

		/// \brief Protect sessions_
		std::shared_mutex mutex_;

		/// \brief List of sessions registered by open, erased by close
		std::set< std::weak_ptr< websocket_session > > sessions_;
	};


	websocket_service::websocket_service()
		impl_(std::make_unique< websocket_service_impl >(*this)){}


	websocket_service::~websocket_service(){}


	void websocket_service::on_open(std::size_t const identifier){}

	void websocket_service::on_close(std::size_t const identifier){}

	void websocket_service::on_text(
		std::size_t const identifier,
		std::string&& data){}

	void websocket_service::on_binary(
		std::size_t const identifier,
		std::vector< std::uint8_t >&& data){}



	void websocket_service::send_text(std::string data){
		impl_->send_text(std::move(data));
	}

	void websocket_service::send_text(
		std::size_t const identifier,
		std::string data
	){
		impl_->send_text(identifier, std::move(data));
	}

	void websocket_service::send_binary(
		std::vector< std::uint8_t > data
	){
		impl_->send_binary(std::move(data));
	}

	void websocket_service::send_binary(
		std::size_t const identifier,
		std::vector< std::uint8_t > data
	){
		impl_->send_binary(identifier, std::move(data));
	}


}
