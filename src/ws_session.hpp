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

#include <webservice/async_lock.hpp>
#include <webservice/ws_handler_location.hpp>
#include <webservice/ws_client_location.hpp>
#include <webservice/shared_const_buffer.hpp>

#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/websocket.hpp>

#include <boost/asio/strand.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/bind_executor.hpp>

#include <boost/circular_buffer.hpp>

#include <boost/any.hpp>

#include <memory>
#include <chrono>
#include <mutex>


namespace webservice{


	class binary_tag{};
	class text_tag{};

	class ws_handler_base;
	class ws_client_base;
	class ws_server_session;
	class ws_client_session;

	using ws_stream
		= boost::beast::websocket::stream< boost::asio::ip::tcp::socket >;

	using ws_strand
		= boost::asio::strand< boost::asio::io_context::executor_type >;

	using http_request
		= boost::beast::http::request< boost::beast::http::string_body >;


	template < typename Derived >
	struct session_location_type;

	template <>
	struct session_location_type< ws_server_session >{
		using type = ws_handler_location;
	};

	template <>
	struct session_location_type< ws_client_session >{
		using type = ws_client_location;
	};


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
		template < typename Tag >
		void send(std::tuple< Tag, shared_const_buffer > data)noexcept;

		/// \brief Close the session
		void send(boost::beast::websocket::close_reason reason)noexcept;

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

		void do_write();

		using location_type = typename session_location_type< Derived >::type;

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


	class ws_server_session: public ws_session< ws_server_session >{
	public:
		/// \brief Take ownership of the socket
		explicit ws_server_session(
			ws_stream&& ws,
			ws_handler_base& service,
			std::chrono::milliseconds ping_time);

		/// \brief Destructor
		~ws_server_session();


		/// \brief Start the asynchronous operation
		void do_accept(http_request&& req);


		/// \brief Called with when a sessions starts
		void on_open()noexcept;

		/// \brief Called with when a sessions ends
		void on_close()noexcept;

		/// \brief Called when a text message
		void on_text(boost::beast::multi_buffer&& buffer)noexcept;

		/// \brief Called when a binary message
		void on_binary(boost::beast::multi_buffer&& buffer)noexcept;

		/// \brief Called when an error occured
		void on_error(
			ws_handler_location location,
			boost::system::error_code ec)noexcept;

		/// \brief Called when an exception was thrown
		void on_exception(std::exception_ptr error)noexcept;


		/// \brief Remove session from handler list
		void remove()noexcept;


	private:
		ws_handler_base& service_;

		std::string resource_;
		bool is_open_ = false;
	};


	class ws_client_session: public ws_session< ws_client_session >{
	public:
		/// \brief Take ownership of the socket
		explicit ws_client_session(
			ws_stream&& ws,
			ws_client_base& client,
			std::chrono::milliseconds ping_time);

		/// \brief Destructor
		~ws_client_session();


		/// \brief Start the session
		void start();


		/// \brief Called when the sessions start
		void on_open()noexcept;

		/// \brief Called when the sessions ends
		void on_close()noexcept;

		/// \brief Called when the session received a text message
		void on_text(boost::beast::multi_buffer&& buffer)noexcept;

		/// \brief Called when the session received a binary message
		void on_binary(boost::beast::multi_buffer&& buffer)noexcept;

		/// \brief Called when an error occured
		void on_error(
			ws_client_location location,
			boost::system::error_code ec)noexcept;

		/// \brief Called when an exception was thrown
		void on_exception(std::exception_ptr error)noexcept;


		/// \brief Remove session on client
		void remove()noexcept;


	private:
		ws_client_base& client_;
		bool is_open_ = false;
	};


}


#endif
