//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__ws_handler_base__hpp_INCLUDED_
#define _webservice__ws_handler_base__hpp_INCLUDED_

#include "shared_const_buffer.hpp"
#include "ws_handler_location.hpp"

#include <boost/beast/core/multi_buffer.hpp>
#include <boost/beast/core/string.hpp>

#include <boost/any.hpp>

#include <string>
#include <chrono>


namespace webservice{


	class ws_server_session;

	class ws_handler_base{
	public:
		ws_handler_base() = default;

		/// \brief Destructor
		virtual ~ws_handler_base();

		ws_handler_base(ws_handler_base const&) = delete;

		ws_handler_base& operator=(ws_handler_base const&) = delete;


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


		/// \brief Called with a unique identifier when a sessions starts
		///
		/// Default implementation does nothing.
		virtual void on_open(
			ws_server_session* session,
			std::string const& resource);

		/// \brief Called with a unique identifier when a sessions ends
		///
		/// Default implementation does nothing.
		virtual void on_close(
			ws_server_session* session,
			std::string const& resource);

		/// \brief Called when a session received a text message
		///
		/// Default implementation does nothing.
		virtual void on_text(
			ws_server_session* session,
			std::string const& resource,
			boost::beast::multi_buffer const& buffer);

		/// \brief Called when a session received a binary message
		///
		/// Default implementation does nothing.
		virtual void on_binary(
			ws_server_session* session,
			std::string const& resource,
			boost::beast::multi_buffer const& buffer);

		/// \brief Called when an error occured
		///
		/// Default implementation does nothing.
		virtual void on_error(
			ws_server_session* session,
			std::string const& resource,
			ws_handler_location location,
			boost::system::error_code ec);

		/// \brief Called when an exception was thrown
		///
		/// Default implementation does nothing.
		virtual void on_exception(
			ws_server_session* session,
			std::string const& resource,
			std::exception_ptr error)noexcept;


		/// \brief Set the owning server
		void set_server(class server& server)noexcept;


	protected:
		/// \brief Get reference to const server
		///
		/// Must not be called before a server is initialized with this service.
		class server const& server()const noexcept{
			assert(server_ != nullptr);
			return *server_;
		}

		/// \brief Get reference to server
		///
		/// Must not be called before a server is initialized with this service.
		class server& server()noexcept{
			assert(server_ != nullptr);
			return *server_;
		}


	private:
		/// \brief Pointer to the server object
		class server* server_ = nullptr;
	};


}


#endif
