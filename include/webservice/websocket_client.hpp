//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__websocket_client__hpp_INCLUDED_
#define _webservice__websocket_client__hpp_INCLUDED_

#include <boost/beast/core/multi_buffer.hpp>
#include <boost/beast/core/string.hpp>

#include <memory>
#include <string>
#include <vector>


namespace webservice{


	class websocket_client_impl;

	class websocket_client{
	public:
		/// \brief Constructor
		websocket_client(
			boost::asio::ip::address address,
			std::uint16_t port,
			std::string resource
		);

		/// \brief Destructor
		virtual ~websocket_client();


		/// \brief Send a text message
		void send_text(std::string data);

		/// \brief Send a binary message
		void send_binary(std::vector< std::uint8_t > data);

		/// \brief Close the sessions
		void close(boost::beast::string_view reason);


	protected:
		/// \brief Called when the sessions starts
		virtual void on_open();

		/// \brief Called when the sessions ends
		virtual void on_close(boost::beast::string_view reason);

		/// \brief Called when the session received a text message
		virtual void on_text(boost::beast::multi_buffer& buffer);

		/// \brief Called when the session received a binary message
		virtual void on_binary(boost::beast::multi_buffer& buffer);


	private:
		/// \brief Pointer to implementation
		std::unique_ptr< websocket_client_impl > impl_;

		friend class websocket_client_impl;
	};


}


#endif
