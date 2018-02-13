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
			std::set< std::size_t > const& identifier,
			std::vector< std::uint8_t > data);


	protected:
		/// \brief Called with a unique identifier when a sessions starts
		virtual void on_open(std::size_t identifier);

		/// \brief Called with a unique identifier when a sessions ends
		virtual void on_close(std::size_t identifier);

		/// \brief Called when a session received a text message
		virtual void on_text(
			std::size_t identifier,
			boost::beast::multi_buffer& buffer);

		/// \brief Called when a session received a binary message
		virtual void on_binary(
			std::size_t identifier,
			boost::beast::multi_buffer& buffer);


	private:
		/// \brief Pointer to implementation
		std::unique_ptr< websocket_service_impl > impl_;

		friend class websocket_service_impl;
		friend class websocket_session;
	};


}


#endif
