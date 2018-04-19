//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__ws_session__hpp_INCLUDED_
#define _webservice__ws_session__hpp_INCLUDED_

#include "async_lock.hpp"
#include "shared_const_buffer.hpp"

#include <boost/beast/websocket.hpp>

#include <boost/asio/strand.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/steady_timer.hpp>

#include <boost/circular_buffer.hpp>

#include <memory>
#include <chrono>


namespace webservice{


	using http_request
		= boost::beast::http::request< boost::beast::http::string_body >;

	using ws_stream
		= boost::beast::websocket::stream< boost::asio::ip::tcp::socket >;

	using ws_strand
		= boost::asio::strand< boost::asio::io_context::executor_type >;


	/// \brief Base of WebSocket sessions
	template < typename Derived >
	class ws_session{
	public:
		/// \brief Take ownership of the socket
		explicit ws_session(
			ws_stream&& ws,
			std::chrono::milliseconds ping_time);

		ws_session(ws_session const&) = delete;

		ws_session& operator=(ws_session const&) = delete;

		/// \brief Async wait on timer
		///
		/// The timer is restarted after any received message.
		///
		/// Send a ping after the first timeout. If it timeouts a second time
		/// after that, close the session.
		void do_timer(char const* op);

		/// \brief Called to indicate activity from the remote peer
		void activity();

		/// \brief Read another message
		void do_read(char const* position);

		/// \brief Called when a message was written
		void on_write(boost::system::error_code ec);

		/// \brief Send a message
		void send(bool is_text, shared_const_buffer buffer)noexcept;

		/// \brief Close the session
		void close(boost::beast::websocket::close_reason reason)noexcept;

		/// \brief Set timers expires_after
		void restart_timer(char const* op);

		/// \brief Close session on socket level
		void close_socket()noexcept;

		/// \brief Stop the timer
		void stop_timer()noexcept;


	protected:
		/// \brief The websocket stream
		ws_stream ws_;

		/// \brief Serialized operations on websocket
		ws_strand strand_;

		/// \brief Serialized call of the handlers
		ws_strand handler_strand_;

		/// \brief Send ping after timeout, close session after second timeout
		boost::asio::steady_timer timer_;

		/// \brief Protectes async operations
		async_locker locker_;


	private:
		/// \brief This as actual type
		Derived& derived(){
			return static_cast< Derived& >(*this);
		}

		/// \brief Send the next outstanding message or close
		void do_write();


		struct write_data{
			bool is_text;
			shared_const_buffer data;
		};

		boost::circular_buffer< write_data > write_list_;

		std::unique_ptr< boost::beast::websocket::close_reason > close_reason_;

		std::chrono::milliseconds const ping_time_;

		boost::beast::multi_buffer buffer_;

		std::size_t ping_counter_;

		bool wait_on_pong_{false};
	};


}


#endif