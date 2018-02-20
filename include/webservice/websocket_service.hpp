//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__websocket_service__hpp_INCLUDED_
#define _webservice__websocket_service__hpp_INCLUDED_

#include <boost/beast/core/multi_buffer.hpp>
#include <boost/beast/core/string.hpp>

#include <memory>
#include <string>
#include <vector>
#include <set>


namespace webservice{


	class websocket_service_impl;

	template < typename Service >
	class websocket_session;

	class server;


	class websocket_service{
	public:
		/// \brief Constructor
		websocket_service();

		/// \brief Destructor
		virtual ~websocket_service();


		/// \brief Send a text message to all sessions
		void send_text(std::string data);

		/// \brief Send a text message to session by identifier
		void send_text(
			std::uintptr_t identifier,
			std::string data);

		/// \brief Send a text message to all sessions by identifier
		void send_text(
			std::set< std::uintptr_t > const& identifier,
			std::string data);


		/// \brief Send a binary message to all sessions
		void send_binary(std::vector< std::uint8_t > data);

		/// \brief Send a binary message to session by identifier
		void send_binary(
			std::uintptr_t identifier,
			std::vector< std::uint8_t > data);

		/// \brief Send a binary message to all sessions by identifier
		void send_binary(
			std::set< std::uintptr_t > const& identifier,
			std::vector< std::uint8_t > data);


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
			boost::beast::multi_buffer& buffer);

		/// \brief Called when a session received a binary message
		///
		/// Default implementation does nothing.
		virtual void on_binary(
			std::uintptr_t identifier,
			std::string const& resource,
			boost::beast::multi_buffer& buffer);

		/// \brief Called when an error occured
		///
		/// Default implementation does nothing.
		virtual void on_error(
			std::uintptr_t identifier,
			std::string const& resource,
			boost::system::error_code ec);

		/// \brief Called when an exception was thrown
		///
		/// Default implementation does nothing.
		virtual void on_exception(
			std::uintptr_t identifier,
			std::string const& resource,
			std::exception_ptr error)noexcept;


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
		/// \brief Pointer to implementation
		std::unique_ptr< websocket_service_impl > impl_;

		/// \brief Pointer to the server object
		class server* server_ = nullptr;

		friend class server;
		friend class websocket_service_impl;
		friend class websocket_session< websocket_service >;
	};


}


#endif
