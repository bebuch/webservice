//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__checked_ws_handler_base__hpp_INCLUDED_
#define _webservice__checked_ws_handler_base__hpp_INCLUDED_

#include "ws_handler_base.hpp"

#include <memory>
#include <set>


namespace webservice{


#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Woverloaded-virtual"
#endif
	class checked_ws_handler_base: public ws_handler_base{
	public:
		checked_ws_handler_base();

		/// \brief Destructor
		~checked_ws_handler_base()override;


		/// \brief Send a text message to all sessions
		void send_text(shared_const_buffer buffer);

		/// \brief Send a text message to session by identifier
		void send_text(
			std::uintptr_t identifier,
			shared_const_buffer buffer);

		/// \brief Send a text message to all sessions by identifier
		void send_text(
			std::set< std::uintptr_t > const& identifier,
			shared_const_buffer buffer);


		/// \brief Send a binary message to all sessions
		void send_binary(shared_const_buffer buffer);

		/// \brief Send a binary message to session by identifier
		void send_binary(
			std::uintptr_t identifier,
			shared_const_buffer buffer);

		/// \brief Send a binary message to all sessions by identifier
		void send_binary(
			std::set< std::uintptr_t > const& identifier,
			shared_const_buffer buffer);


		/// \brief Shutdown all sessions
		void close(boost::beast::string_view reason);

		/// \brief Shutdown session by identifier
		void close(
			std::uintptr_t identifier,
			boost::beast::string_view reason);

		/// \brief Shutdown all sessions by identifier
		void close(
			std::set< std::uintptr_t > const& identifier,
			boost::beast::string_view reason);


		/// \brief Don't accept new connections, send close to all existing
		///        connections
		void shutdown()noexcept override;


	private:
		/// \brief Don't accept new connections, send close to all existing
		///        connections
		void do_shutdown()noexcept;

		/// \brief Called with a unique identifier when a sessions starts
		///
		/// Default implementation does nothing.
		virtual void on_open(
			std::uintptr_t identifier,
			std::string const& resource);

		/// \brief Called with a unique identifier when a sessions ends
		///
		/// Default implementation does nothing.
		virtual void on_close(
			std::uintptr_t identifier,
			std::string const& resource);

		/// \brief Called when a session received a text message
		///
		/// Default implementation does nothing.
		virtual void on_text(
			std::uintptr_t identifier,
			std::string const& resource,
			boost::beast::multi_buffer const& buffer);

		/// \brief Called when a session received a binary message
		///
		/// Default implementation does nothing.
		virtual void on_binary(
			std::uintptr_t identifier,
			std::string const& resource,
			boost::beast::multi_buffer const& buffer);

		/// \brief Called when an error occured
		///
		/// Default implementation does nothing.
		virtual void on_error(
			std::uintptr_t identifier,
			std::string const& resource,
			ws_handler_location location,
			boost::system::error_code ec);

		/// \brief Called when an exception was thrown
		///
		/// Default implementation does nothing.
		virtual void on_exception(
			std::uintptr_t identifier,
			std::string const& resource,
			std::exception_ptr error)noexcept;


		/// \brief Called with a unique identifier when a sessions starts
		///
		/// Default implementation does nothing.
		void on_open(
			ws_server_session* session,
			std::string const& resource)final;

		/// \brief Called with a unique identifier when a sessions ends
		///
		/// Default implementation does nothing.
		void on_close(
			ws_server_session* session,
			std::string const& resource)final;

		/// \brief Called when a session received a text message
		///
		/// Default implementation does nothing.
		void on_text(
			ws_server_session* session,
			std::string const& resource,
			boost::beast::multi_buffer const& buffer)final;

		/// \brief Called when a session received a binary message
		///
		/// Default implementation does nothing.
		void on_binary(
			ws_server_session* session,
			std::string const& resource,
			boost::beast::multi_buffer const& buffer)final;

		/// \brief Called when an error occured
		///
		/// Default implementation does nothing.
		void on_error(
			ws_server_session* session,
			std::string const& resource,
			ws_handler_location location,
			boost::system::error_code ec)final;

		/// \brief Called when an exception was thrown
		///
		/// Default implementation does nothing.
		void on_exception(
			ws_server_session* session,
			std::string const& resource,
			std::exception_ptr error)noexcept final;


		/// \brief Pointer to implementation
		std::unique_ptr< class checked_ws_handler_base_impl > impl_;

		friend class checked_ws_handler_base_impl;
	};
#ifdef __clang__
#pragma clang diagnostic pop
#endif


}


#endif
