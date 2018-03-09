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

#include "ws_handler_base.hpp"

#include <memory>
#include <set>


namespace webservice{


	class ws_service_base{
	public:
		/// \brief Destructor
		virtual ~ws_service_base();

		/// \brief Send a text message to session
		void send_text(
			ws_server_session* session,
			shared_const_buffer buffer);

		/// \brief Send a binary message to session
		void send_binary(
			ws_server_session* session,
			shared_const_buffer buffer);

		/// \brief Shutdown session
		void close(
			ws_server_session* session,
			boost::beast::string_view reason);


	protected:
		/// \brief Called with a unique identifier when a sessions starts
		///
		/// Default implementation does nothing.
		virtual void on_open(ws_server_session* session);

		/// \brief Called with a unique identifier when a sessions ends
		///
		/// Default implementation does nothing.
		virtual void on_close(ws_server_session* session);

		/// \brief Called when a session received a text message
		///
		/// Default implementation does nothing.
		virtual void on_text(
			ws_server_session* session,
			boost::beast::multi_buffer const& buffer);

		/// \brief Called when a session received a binary message
		///
		/// Default implementation does nothing.
		virtual void on_binary(
			ws_server_session* session,
			boost::beast::multi_buffer const& buffer);

		/// \brief Called when an error occured
		///
		/// Default implementation does nothing.
		virtual void on_error(
			ws_server_session* session,
			ws_handler_location location,
			boost::system::error_code ec);

		/// \brief Called when an exception was thrown
		///
		/// Default implementation does nothing.
		virtual void on_exception(
			ws_server_session* session,
			std::exception_ptr error)noexcept;

		friend class service_ws_handler;
	};


}


#endif
