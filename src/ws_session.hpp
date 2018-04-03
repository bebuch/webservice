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

#include "sessions.hpp"

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

		/// \brief Destructor
		~ws_session();

		/// \brief Async wait on timer
		///
		/// The timer is restarted after any received message.
		///
		/// Send a ping after the first timeout. If it timeouts a second time
		/// after that, close the session.
		void do_timer();

		/// \brief Called to indicate activity from the remote peer
		void activity();

		/// \brief Read another message
		void do_read();

		/// \brief Called when a message was written
		void on_write(boost::system::error_code ec);

		/// \brief Send a message
		template < typename Tag >
		void send(std::tuple< Tag, shared_const_buffer > data);

		/// \brief Close the session
		void send(boost::beast::websocket::close_reason reason);

		/// \brief Set timers expires_after
		void restart_timer();


		/// \brief Set the function that is called on async_erase
		void set_erase_fn(sessions_erase_fn< Derived >&& erase_fn)noexcept;

		/// \brief Send a request to erase this session from the list
		///
		/// The request is sended only once, any call after the fist will be
		/// ignored.
		void async_erase();


	protected:
		/// \brief The websocket stream
		ws_stream ws_;

		ws_strand strand_;

		ws_strand handler_strand_;

		sessions_erase_fn< Derived > erase_fn_;

		std::once_flag erase_flag_;
		std::atomic< std::size_t > async_calls_{0};


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

		std::chrono::milliseconds const ping_time_;

		boost::asio::steady_timer timer_;
		boost::beast::multi_buffer buffer_;

		std::size_t ping_counter_;

		std::atomic< bool > wait_on_pong_{false};
	};


	class ws_server_session: public ws_session< ws_server_session >{
	public:
		/// \brief Take ownership of the socket
		explicit ws_server_session(
			ws_stream&& ws,
			class server& server,
			std::chrono::milliseconds ping_time);

		/// \brief Destructor
		~ws_server_session();


		/// \brief Start the asynchronous operation
		void do_accept(http_request&& req);

		/// \brief Called when do_accept is ready
		void on_accept(boost::system::error_code ec);


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


		/// \brief Bind session to another ws_handler
		void rebind(ws_handler_base* service)noexcept;


	private:
		class server_impl& server_;
		std::atomic< ws_handler_base* > service_;
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


	private:
		ws_client_base& client_;
		bool is_open_ = false;
	};


}


#endif
