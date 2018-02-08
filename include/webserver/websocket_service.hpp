//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webserver
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webserver__websocket_service__hpp_INCLUDED_
#define _webserver__websocket_service__hpp_INCLUDED_

#include <string>
#include <vector>


namespace webserver{


	class websocket_service{
	public:
		/// \brief Constructor
		websocket_service();

		/// \brief Destructor
		virtual ~websocket_service();


		/// \brief Send a text message to all sessions
		void send_text(std::string const& data);

		/// \brief Send a text message to session by identifier
		void send_text(
			std::size_t identifier,
			std::string data);

		/// \brief Send a text message to all sessions by identifier
		void send_text(
			std::set< std::size_t > const& identifier,
			std::string data);

		/// \brief Send a binary message to all sessions
		void send_binary(std::vector< std::uint8_t > data);

		/// \brief Send a binary message to session by identifier
		void send_binary(
			std::size_t identifier,
			std::vector< std::uint8_t > data);

		/// \brief Send a binary message to all sessions by identifier
		void send_binary(
			std::set< std::size_t > identifier,
			std::vector< std::uint8_t > data);


	protected:
		/// \brief Called with a unique identifier when a sessions starts
		virtual void on_open(std::size_t identifier);

		/// \brief Called with a unique identifier when a sessions ends
		virtual void on_close(std::size_t identifier);

		/// \brief Called when a session received a text message
		virtual void on_text(
			std::size_t identifier,
			std::string&& data);

		/// \brief Called when a session received a binary message
		virtual void on_binary(
			std::size_t identifier,
			std::vector< std::uint8_t >&& data);


	private:
		/// \brief Pointer to implementation
		std::unique_ptr< websocket_service_impl > impl_;

		friend class websocket_service_impl;
	};


}


#endif
