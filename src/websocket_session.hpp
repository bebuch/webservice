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

#include <webservice/websocket_service_error.hpp>

#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/websocket.hpp>

#include <boost/asio/strand.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/bind_executor.hpp>

#include <memory>


namespace webservice{


	class websocket_service;
	class websocket_client;
	class websocket_server_session;

	template < typename Derived >
	struct session_error_type;

	template <>
	struct session_error_type< websocket_server_session >{
		using type = websocket_service_error;
	};

	template < typename Derived >
	struct websocket_session_callbacks{
		/// \brief Called with a unique identifier when a sessions starts
		void on_open()noexcept{
			try{
				static_cast< Derived* >(this)->on_open();
			}catch(...){
				on_exception(std::current_exception());
			}
		}

		/// \brief Called with a unique identifier when a sessions ends
		void on_close()noexcept{
			try{
				static_cast< Derived* >(this)->on_close();
			}catch(...){
				on_exception(std::current_exception());
			}
		}

		/// \brief Called when a session received a text message
		void on_text(boost::beast::multi_buffer& buffer)noexcept{
			try{
				static_cast< Derived* >(this)->on_text(buffer);
			}catch(...){
				on_exception(std::current_exception());
			}
		}

		/// \brief Called when a session received a binary message
		void on_binary(boost::beast::multi_buffer& buffer)noexcept{
			try{
				static_cast< Derived* >(this)->on_binary(buffer);
			}catch(...){
				on_exception(std::current_exception());
			}
		}

		/// \brief Called when an error occured
		template < typename Error >
		void on_error(Error error, boost::system::error_code ec)noexcept{
			try{
				static_cast< Derived* >(this)->on_error(error, ec);
			}catch(...){
				on_exception(std::current_exception());
			}
		}

		/// \brief Called when an exception was thrown
		void on_exception(std::exception_ptr error)noexcept{
			static_cast< Derived* >(this)->on_exception(error);
		}
	};


	/// \brief Base of WebSocket sessions
	template < typename Derived >
	class websocket_session
		: protected websocket_session_callbacks< Derived >
		, public std::enable_shared_from_this< Derived >{
	public:
		/// \brief Take ownership of the socket
		explicit websocket_session(boost::asio::ip::tcp::socket socket);

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

		/// \brief Set timers expires_after
		void start_timer();


	protected:
		boost::beast::websocket::stream< boost::asio::ip::tcp::socket > ws_;
		boost::asio::strand< boost::asio::io_context::executor_type > strand_;


	private:
		using error_type = typename session_error_type< Derived >::type;

		boost::asio::steady_timer timer_;
		boost::beast::multi_buffer buffer_;
		char ping_state_ = 0;
	};


	class websocket_server_session
		: public websocket_session< websocket_server_session >{
	public:
		/// \brief Take ownership of the socket
		explicit websocket_server_session(
			boost::asio::ip::tcp::socket socket,
			websocket_service& service);

		/// \brief Destructor
		~websocket_server_session();


		/// \brief Start the asynchronous operation
		void do_accept(
			boost::beast::http::request< boost::beast::http::string_body > req
		);

		/// \brief Called when do_accept is ready
		void on_accept(boost::system::error_code ec);


	private:
		/// \brief Called with a unique identifier when a sessions starts
		void on_open();

		/// \brief Called with a unique identifier when a sessions ends
		void on_close();

		/// \brief Called when a session received a text message
		void on_text(boost::beast::multi_buffer& buffer);

		/// \brief Called when a session received a binary message
		void on_binary(boost::beast::multi_buffer& buffer);

		/// \brief Called when an error occured
		void on_error(
			websocket_service_error error,
			boost::system::error_code ec);

		/// \brief Called when an exception was thrown
		void on_exception(std::exception_ptr error)noexcept;


		websocket_service& service_;
		std::string resource_;
		bool is_open_ = false;


		friend class websocket_session_callbacks< websocket_server_session >;
	};



}


#endif
