//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__ws_client__hpp_INCLUDED_
#define _webservice__ws_client__hpp_INCLUDED_

#include <webservice/ws_client_error.hpp>

#include <boost/beast/core/multi_buffer.hpp>
#include <boost/beast/core/string.hpp>

#include <memory>
#include <string>
#include <vector>


namespace webservice{


	class ws_client{
	public:
		/// \brief Constructor
		ws_client(
			std::string host,
			std::string port,
			std::string resource
		);

		/// \brief Destructor
		virtual ~ws_client();


		/// \brief Connect client to server
		///
		/// Does nothing if client is already connected.
		void connect();

		/// \brief true if client is connected to server, false otherwise
		bool is_connected()const;


		/// \brief Send a text message
		void send_text(std::string data);

		/// \brief Send a binary message
		void send_binary(std::vector< std::uint8_t > data);


		/// \brief Close the sessions
		void close(boost::beast::string_view reason);


	protected:
		/// \brief Called when the sessions starts
		///
		/// Default implementation does nothing.
		virtual void on_open();

		/// \brief Called when the sessions ends
		///
		/// Default implementation does nothing.
		virtual void on_close();

		/// \brief Called when the session received a text message
		///
		/// Default implementation does nothing.
		virtual void on_text(boost::beast::multi_buffer& buffer);

		/// \brief Called when the session received a binary message
		///
		/// Default implementation does nothing.
		virtual void on_binary(boost::beast::multi_buffer& buffer);

		/// \brief Called when an error occured
		///
		/// Default implementation does nothing.
		virtual void on_error(
			ws_client_error error,
			boost::system::error_code ec);

		/// \brief Called when an exception was thrown
		///
		/// Default implementation does nothing.
		virtual void on_exception(std::exception_ptr error)noexcept;


	private:
		/// \brief Pointer to implementation
		std::unique_ptr< class ws_client_impl > impl_;

		friend class ws_client_impl;
	};


}


#endif