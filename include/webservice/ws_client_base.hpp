//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__ws_client_base__hpp_INCLUDED_
#define _webservice__ws_client_base__hpp_INCLUDED_

#include <webservice/ws_client_location.hpp>

#include <boost/beast/core/multi_buffer.hpp>
#include <boost/beast/core/string.hpp>

#include <boost/any.hpp>

#include <memory>
#include <string>
#include <vector>


namespace webservice{


	class ws_client_base{
	public:
		/// \brief Constructor
		ws_client_base(
			std::string host,
			std::string port,
			std::string resource
		);

		/// \brief Destructor
		virtual ~ws_client_base();


		/// \brief Connect client to server
		///
		/// Does nothing if client is already connected.
		void connect();

		/// \brief true if client is connected to server, false otherwise
		bool is_connected()const;


		/// \brief Send a text message
		void send_text(
			boost::asio::const_buffer const& buffer,
			std::shared_ptr< boost::any > data);

		/// \brief Send a binary message
		void send_binary(
			boost::asio::const_buffer const& buffer,
			std::shared_ptr< boost::any > data);


		/// \brief Close the sessions
		void close(boost::beast::string_view reason);


		/// \brief Wait on the processing threads
		///
		/// This effecivly blocks the current thread until the client is closed.
		void block()noexcept;

		/// \brief Close the connection as fast as possible
		///
		/// This function is not blocking. Call block() if you want to wait
		/// until all connections are closed.
		void stop()noexcept;


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
		virtual void on_text(boost::beast::multi_buffer const& buffer);

		/// \brief Called when the session received a binary message
		///
		/// Default implementation does nothing.
		virtual void on_binary(boost::beast::multi_buffer const& buffer);

		/// \brief Called when an error occured
		///
		/// Default implementation does nothing.
		virtual void on_error(
			ws_client_location location,
			boost::system::error_code ec);

		/// \brief Called when an exception was thrown
		///
		/// Default implementation does nothing.
		virtual void on_exception(std::exception_ptr error)noexcept;


	private:
		/// \brief Pointer to implementation
		std::unique_ptr< class ws_client_base_impl > impl_;

		friend class ws_client_base_impl;
	};


}


#endif
