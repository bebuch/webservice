//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__ws_handler_base__hpp_INCLUDED_
#define _webservice__ws_handler_base__hpp_INCLUDED_

#include "shared_const_buffer.hpp"
#include "ws_handler_location.hpp"

#include <boost/beast/core/multi_buffer.hpp>
#include <boost/beast/core/string.hpp>

#include <boost/any.hpp>

#include <string>
#include <chrono>


namespace webservice{


	class ws_server_session;

	class ws_handler_base{
	public:
		ws_handler_base() = default;

		/// \brief Destructor
		virtual ~ws_handler_base();

		ws_handler_base(ws_handler_base const&) = delete;

		ws_handler_base& operator=(ws_handler_base const&) = delete;


		/// \brief Send a text message to session
		void send_text(
			ws_server_session* session,
			shared_const_buffer buffer);

		/// \brief Send a binary message to session
		void send_binary(
			ws_server_session* session,
			shared_const_buffer buffer);

		/// \brief Shutdown session
		void close(
			ws_server_session* session,
			boost::beast::string_view reason);


		/// \brief Called with a unique identifier when a sessions starts
		///
		/// Default implementation does nothing.
		virtual void on_open(
			ws_server_session* session,
			std::string const& resource);

		/// \brief Called with a unique identifier when a sessions ends
		///
		/// Default implementation does nothing.
		virtual void on_close(
			ws_server_session* session,
			std::string const& resource);

		/// \brief Called when a session received a text message
		///
		/// Default implementation does nothing.
		virtual void on_text(
			ws_server_session* session,
			std::string const& resource,
			boost::beast::multi_buffer&& buffer);

		/// \brief Called when a session received a binary message
		///
		/// Default implementation does nothing.
		virtual void on_binary(
			ws_server_session* session,
			std::string const& resource,
			boost::beast::multi_buffer&& buffer);

		/// \brief Called when an error occured
		///
		/// Default implementation does nothing.
		virtual void on_error(
			ws_server_session* session,
			std::string const& resource,
			ws_handler_location location,
			boost::system::error_code ec);

		/// \brief Called when an exception was thrown
		///
		/// Default implementation does nothing.
		virtual void on_exception(
			ws_server_session* session,
			std::string const& resource,
			std::exception_ptr error)noexcept;


		/// \brief Shutdown hint called by the server
		virtual void shutdown()noexcept;


		/// \brief Set the owning server
		virtual void set_server(class server* server);


		/// \brief Set max size of incomming WebSocket messages
		void set_max_read_message_size(std::size_t bytes){
			max_read_message_size_ = bytes;
		}

		/// \brief Max size of incomming WebSocket messages
		std::size_t max_read_message_size()const{
			return max_read_message_size_;
		}


		/// \brief Set session timeout
		void set_websocket_ping_time(std::chrono::milliseconds ms){
			websocket_ping_time_ = ms;
		}

		/// \brief No session timeout
		void unset_websocket_ping_time(){
			websocket_ping_time_.reset();
		}

		/// \brief Session timeout
		boost::optional< std::chrono::milliseconds > websocket_ping_time()const{
			return websocket_ping_time_;
		}


	protected:
		/// \brief Get reference to server
		class server* server()noexcept{
			return server_;
		}


	private:
		/// \brief Pointer to the server object
		class server* server_ = nullptr;

		/// \brief Max size of incomming http and WebSocket messages
		std::size_t max_read_message_size_{16 * 1024 * 1024};

		/// \brief WebSocket session timeout
		///
		/// If empty, WebSocket sessions don't ping and have no timeout.
		///
		/// If set, after this time without an incomming message a ping is send.
		/// If no message is incomming after a second period of this time, the
		/// session is considerd to be dead and will be closed.
		boost::optional< std::chrono::milliseconds > websocket_ping_time_{};

	};


}


#endif
