//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__websocket_session__hpp_INCLUDED_
#define _webservice__websocket_session__hpp_INCLUDED_

#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/websocket.hpp>

#include <boost/asio/strand.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/bind_executor.hpp>

#include <memory>


namespace webservice{


	/// \brief Echoes back all received WebSocket messages
	template < typename Service >
	class websocket_session
		: public std::enable_shared_from_this< websocket_session< Service > >
	{
	public:
		/// \brief Take ownership of the socket
		explicit websocket_session(
			boost::asio::ip::tcp::socket socket,
			Service& service);

		/// \brief Destructor
		~websocket_session();

		/// \brief Start the asynchronous operation
		void do_accept(
			boost::beast::http::request< boost::beast::http::string_body > req
		);

		/// \brief Called when do_accept is ready
		void on_accept(boost::system::error_code ec);

		/// \brief Called when the timer expires.
		void on_timer(boost::system::error_code ec);

		/// \brief Called to indicate activity from the remote peer
		void activity();

		/// \brief Called after a ping is sent.
		void on_ping(boost::system::error_code ec);

		/// \brief Read another message
		void do_read();

		/// \brief Called when a message was readed
		void on_read(boost::system::error_code ec);

		/// \brief Called when a message was written
		void on_write(boost::system::error_code ec);

		/// \brief Send a message
		template < typename Data >
		void send(std::shared_ptr< Data > data);

		/// \brief Close the session
		void send(boost::beast::websocket::close_reason reason);

		/// \brief Close the session
		void close(boost::system::error_code ec);


	private:
		/// \brief Called with a unique identifier when a sessions starts
		void on_open();

		/// \brief Called with a unique identifier when a sessions ends
		void on_close();

		/// \brief Called when a session received a text message
		void on_text(boost::beast::multi_buffer& buffer);

		/// \brief Called when a session received a binary message
		void on_binary(boost::beast::multi_buffer& buffer);

		/// \brief Called when an accept error occured
		void on_accept_error(boost::system::error_code ec);

		/// \brief Called when an timer error occured
		void on_timer_error(boost::system::error_code ec);

		/// \brief Called when an ping error occured
		void on_ping_error(boost::system::error_code ec);

		/// \brief Called when an read error occured
		void on_read_error(boost::system::error_code ec);

		/// \brief Called when an write error occured
		void on_write_error(boost::system::error_code ec);

		/// \brief Called when an exception was thrown
		void on_exception(std::exception_ptr error)noexcept;


		Service& service_;
		boost::beast::websocket::stream< boost::asio::ip::tcp::socket > ws_;
		boost::asio::strand< boost::asio::io_context::executor_type > strand_;
		boost::asio::steady_timer timer_;
		boost::beast::multi_buffer buffer_;
		char ping_state_ = 0;
		std::string resource_;
		bool is_open_ = false;
	};


}


#endif
