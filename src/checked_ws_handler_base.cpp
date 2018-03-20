//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <webservice/checked_ws_handler_base.hpp>
#include <webservice/server.hpp>

#include "ws_session.hpp"

#include <shared_mutex>


namespace webservice{



	/// \brief Implementation of checked_ws_handler_base
	struct checked_ws_handler_base_impl{
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


		/// \brief Send a message to session
		template < typename Data >
		void send(
			ws_server_session* const session,
			Data&& data
		){
			session->send(static_cast< Data&& >(data));
		}


		/// \brief If it is true, new sessions are closed immediately
		std::atomic< bool > shutdown_{false};

		/// \brief Protect sessions_
		///
		/// We don't need the timed-part, but C++14 has no non-timed
		/// shared_mutex.
		std::shared_timed_mutex mutex_;

		/// \brief List of sessions registered by open, erased by close
		std::set< ws_server_session* > sessions_;
	};



	checked_ws_handler_base::checked_ws_handler_base()
		: impl_(std::make_unique< checked_ws_handler_base_impl >()){}


	checked_ws_handler_base::~checked_ws_handler_base(){
		shutdown();
	}


	void checked_ws_handler_base::shutdown()noexcept{
		if(impl_->shutdown_.exchange(true)){
			impl_->send("handler shutdown");
		}

		for(;;){
			std::unique_lock< std::shared_timed_mutex > lock(impl_->mutex_);
			if(impl_->sessions_.empty()){
				break;
			}
			lock.unlock();
			server().run_one();
		}
	}


	void checked_ws_handler_base::on_open(
		std::uintptr_t const /*identifier*/,
		std::string const& /*resource*/){}

	void checked_ws_handler_base::on_close(
		std::uintptr_t const /*identifier*/,
		std::string const& /*resource*/){}

	void checked_ws_handler_base::on_text(
		std::uintptr_t const /*identifier*/,
		std::string const& /*resource*/,
		boost::beast::multi_buffer const& /*buffer*/){}

	void checked_ws_handler_base::on_binary(
		std::uintptr_t const /*identifier*/,
		std::string const& /*resource*/,
		boost::beast::multi_buffer const& /*buffer*/){}

	void checked_ws_handler_base::on_error(
		std::uintptr_t const /*identifier*/,
		std::string const& /*resource*/,
		ws_handler_location /*location*/,
		boost::system::error_code /*ec*/){}

	void checked_ws_handler_base::on_exception(
		std::uintptr_t const /*identifier*/,
		std::string const& /*resource*/,
		std::exception_ptr /*error*/)noexcept{}


	void checked_ws_handler_base::on_open(
		ws_server_session* const session,
		std::string const& resource
	){
		if(!impl_->shutdown_){
			std::unique_lock< std::shared_timed_mutex > lock(impl_->mutex_);
			impl_->sessions_.emplace(session);
			lock.unlock();

			auto identifier = reinterpret_cast< std::uintptr_t >(session);
			on_open(identifier, resource);
		}else{
			session->rebind(nullptr);
			session->send("handler shutdown");
		}
	}

	void checked_ws_handler_base::on_close(
		ws_server_session* const session,
		std::string const& resource
	){
		on_close(reinterpret_cast< std::uintptr_t >(session), resource);

		std::unique_lock< std::shared_timed_mutex > lock(impl_->mutex_);
		impl_->sessions_.erase(session);
	}

	void checked_ws_handler_base::on_text(
		ws_server_session* const session,
		std::string const& resource,
		boost::beast::multi_buffer const& buffer
	){
		on_text(
			reinterpret_cast< std::uintptr_t >(session),
			resource,
			buffer);
	}

	void checked_ws_handler_base::on_binary(
		ws_server_session* const session,
		std::string const& resource,
		boost::beast::multi_buffer const& buffer
	){
		on_binary(
			reinterpret_cast< std::uintptr_t >(session),
			resource,
			buffer);
	}

	void checked_ws_handler_base::on_error(
		ws_server_session* const session,
		std::string const& resource,
		ws_handler_location location,
		boost::system::error_code ec
	){
		on_error(
			reinterpret_cast< std::uintptr_t >(session),
			resource,
			location,
			ec);
	}

	void checked_ws_handler_base::on_exception(
		ws_server_session* session,
		std::string const& resource,
		std::exception_ptr error
	)noexcept{
		on_exception(
			reinterpret_cast< std::uintptr_t >(session),
			resource,
			error);
	}


	void checked_ws_handler_base::send_text(shared_const_buffer buffer){
		impl_->send(std::make_tuple(text_tag{}, std::move(buffer)));
	}

	void checked_ws_handler_base::send_text(
		std::uintptr_t const identifier,
		shared_const_buffer buffer
	){
		impl_->send(
			identifier,
			std::make_tuple(text_tag{}, std::move(buffer)));
	}

	void checked_ws_handler_base::send_text(
		std::set< std::uintptr_t > const& identifier,
		shared_const_buffer buffer
	){
		impl_->send(
			identifier,
			std::make_tuple(text_tag{}, std::move(buffer)));
	}


	void checked_ws_handler_base::send_binary(shared_const_buffer buffer){
		impl_->send(std::make_tuple(binary_tag{}, std::move(buffer)));
	}

	void checked_ws_handler_base::send_binary(
		std::uintptr_t const identifier,
		shared_const_buffer buffer
	){
		impl_->send(
			identifier,
			std::make_tuple(binary_tag{}, std::move(buffer)));
	}

	void checked_ws_handler_base::send_binary(
		std::set< std::uintptr_t > const& identifier,
		shared_const_buffer buffer
	){
		impl_->send(
			identifier,
			std::make_tuple(binary_tag{}, std::move(buffer)));
	}


	void checked_ws_handler_base::close(boost::beast::string_view reason){
		impl_->send(boost::beast::websocket::close_reason(reason));
	}

	void checked_ws_handler_base::close(
		std::uintptr_t identifier,
		boost::beast::string_view reason
	){
		impl_->send(identifier, boost::beast::websocket::close_reason(reason));
	}

	void checked_ws_handler_base::close(
		std::set< std::uintptr_t > const& identifier,
		boost::beast::string_view reason
	){
		impl_->send(identifier, boost::beast::websocket::close_reason(reason));
	}



}
