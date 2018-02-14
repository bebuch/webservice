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
	class websocket_session;

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
		virtual void on_open(
			std::uintptr_t identifier,
			std::string const& resource);

		/// \brief Called with a unique identifier when a sessions ends
		virtual void on_close(
			std::uintptr_t identifier,
			std::string const& resource);

		/// \brief Called when a session received a text message
		virtual void on_text(
			std::uintptr_t identifier,
			std::string const& resource,
			boost::beast::multi_buffer& buffer);

		/// \brief Called when a session received a binary message
		virtual void on_binary(
			std::uintptr_t identifier,
			std::string const& resource,
			boost::beast::multi_buffer& buffer);


	private:
		/// \brief Pointer to implementation
		std::unique_ptr< websocket_service_impl > impl_;

		friend class websocket_service_impl;
		friend class websocket_session;
	};


}


#endif
