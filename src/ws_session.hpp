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

#include <boost/optional.hpp>

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

	using ws_stream =
		boost::beast::websocket::stream< boost::asio::ip::tcp::socket >;

	using ws_strand =
		boost::asio::strand< boost::asio::io_context::executor_type >;


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


	template < typename Derived >
	class ws_session_callbacks{
	protected:
		/// \brief Called when a session starts
		void call_on_open()noexcept{
			try{
				derived()->on_open();
			}catch(...){
				call_on_exception(std::current_exception());
			}
		}

		/// \brief Called when a sessions ends
		void call_on_close()noexcept{
			try{
				derived()->on_close();
			}catch(...){
				call_on_exception(std::current_exception());
			}
		}

		/// \brief Called when a session received a text message
		void call_on_text(boost::beast::multi_buffer const& buffer)noexcept{
			try{
				derived()->on_text(buffer);
			}catch(...){
				call_on_exception(std::current_exception());
			}
		}

		/// \brief Called when a session received a binary message
		void call_on_binary(boost::beast::multi_buffer const& buffer)noexcept{
			try{
				derived()->on_binary(buffer);
			}catch(...){
				call_on_exception(std::current_exception());
			}
		}

		/// \brief Called when an error occured
		template < typename Error >
		void call_on_error(Error error, boost::system::error_code ec)noexcept{
			try{
				derived()->on_error(error, ec);
			}catch(...){
				call_on_exception(std::current_exception());
			}
		}

		/// \brief Called when an exception was thrown
		void call_on_exception(std::exception_ptr error)noexcept{
			derived()->on_exception(error);
		}


	private:
		/// \brief This as actual type
		Derived* derived(){
			return static_cast< Derived* >(this);
		}
	};


	/// \brief Base of WebSocket sessions
	template < typename Derived >
	class ws_session
		: public ws_session_callbacks< Derived >
		, public std::enable_shared_from_this< Derived >{
	public:
		/// \brief Take ownership of the socket
		explicit ws_session(
			ws_stream&& ws,
			boost::optional< std::chrono::milliseconds > websocket_ping_time);

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
		template < typename Tag >
		void send(std::tuple< Tag, shared_const_buffer > data);

		/// \brief Close the session
		void send(boost::beast::websocket::close_reason reason);

		/// \brief Close the session
		void close(boost::system::error_code ec);

		/// \brief Set timers expires_after
		void start_timer();


	protected:
		/// \brief The websocket stream
		ws_stream ws_;

		ws_strand strand_;


	private:
		void do_write();

		using location_type = typename session_location_type< Derived >::type;
		using callback = ws_session_callbacks< Derived >;

		boost::optional< std::chrono::milliseconds > const websocket_ping_time_;

		struct write_data{
			bool is_text;
			shared_const_buffer data;
		};
		std::mutex write_mutex_;
		boost::circular_buffer< write_data > write_list_;

		boost::asio::steady_timer timer_;
		boost::beast::multi_buffer buffer_;
		char ping_state_ = 0;
		std::size_t ping_counter_;
	};


	class ws_server_session: public ws_session< ws_server_session >{
	public:
		/// \brief Take ownership of the socket
		explicit ws_server_session(
			ws_stream&& ws,
			ws_handler_base& service,
			boost::optional< std::chrono::milliseconds > websocket_ping_time);

		/// \brief Destructor
		~ws_server_session();


		/// \brief Start the asynchronous operation
		void do_accept(
			boost::beast::http::request< boost::beast::http::string_body > req
		);

		/// \brief Called when do_accept is ready
		void on_accept(boost::system::error_code ec);


		/// \brief Called with when a sessions starts
		void on_open();

		/// \brief Called with when a sessions ends
		void on_close();

		/// \brief Called when a text message
		void on_text(boost::beast::multi_buffer const& buffer);

		/// \brief Called when a binary message
		void on_binary(boost::beast::multi_buffer const& buffer);

		/// \brief Called when an error occured
		void on_error(
			ws_handler_location location,
			boost::system::error_code ec);

		/// \brief Called when an exception was thrown
		void on_exception(std::exception_ptr error)noexcept;


		/// \brief Bind session to another ws_handler
		void rebind(ws_handler_base* service)noexcept;


	private:
		using callback = ws_session_callbacks< ws_server_session >;

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
			boost::optional< std::chrono::milliseconds > websocket_ping_time);

		/// \brief Destructor
		~ws_client_session();


		/// \brief Start the session
		void start();


		/// \brief Called when the sessions start
		void on_open();

		/// \brief Called when the sessions ends
		void on_close();

		/// \brief Called when the session received a text message
		void on_text(boost::beast::multi_buffer const& buffer);

		/// \brief Called when the session received a binary message
		void on_binary(boost::beast::multi_buffer const& buffer);

		/// \brief Called when an error occured
		void on_error(
			ws_client_location location,
			boost::system::error_code ec);

		/// \brief Called when an exception was thrown
		void on_exception(std::exception_ptr error)noexcept;


	private:
		using callback = ws_session_callbacks< ws_client_session >;

		ws_client_base& client_;
		bool is_open_ = false;
	};


}


#endif
