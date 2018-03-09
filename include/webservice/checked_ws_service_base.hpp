//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__checked_ws_service_base__hpp_INCLUDED_
#define _webservice__checked_ws_service_base__hpp_INCLUDED_

#include "ws_handler_base.hpp"

#include <memory>
#include <set>


namespace webservice{


	class checked_ws_service_base{
	public:
		/// \brief Destructor
		virtual ~checked_ws_service_base();

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


	protected:
		/// \brief Called with a unique identifier when a sessions starts
		///
		/// Default implementation does nothing.
		virtual void on_open(std::uintptr_t identifier);

		/// \brief Called with a unique identifier when a sessions ends
		///
		/// Default implementation does nothing.
		virtual void on_close(std::uintptr_t identifier);

		/// \brief Called when a session received a text message
		///
		/// Default implementation does nothing.
		virtual void on_text(
			std::uintptr_t identifier,
			boost::beast::multi_buffer const& buffer);

		/// \brief Called when a session received a binary message
		///
		/// Default implementation does nothing.
		virtual void on_binary(
			std::uintptr_t identifier,
			boost::beast::multi_buffer const& buffer);

		/// \brief Called when an error occured
		///
		/// Default implementation does nothing.
		virtual void on_error(
			std::uintptr_t identifier,
			ws_handler_location location,
			boost::system::error_code ec);

		/// \brief Called when an exception was thrown
		///
		/// Default implementation does nothing.
		virtual void on_exception(
			std::uintptr_t identifier,
			std::exception_ptr error)noexcept;
	};


}


#endif
