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

#include "shared_const_buffer.hpp"
#include "ws_client_location.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/executor_work_guard.hpp>

#include <boost/beast/core/multi_buffer.hpp>
#include <boost/beast/core/string.hpp>

#include <memory>
#include <chrono>
#include <string>
#include <vector>
#include <thread>
#include <mutex>


namespace webservice{


	using strand
		= boost::asio::strand< boost::asio::io_context::executor_type >;


	class ws_client_base{
	public:
		/// \brief Constructor
		ws_client_base();

		ws_client_base(ws_client_base const&) = delete;

		ws_client_base& operator=(ws_client_base const&) = delete;


		/// \brief Destructor
		virtual ~ws_client_base();


		/// \brief Connect client to server
		void async_connect(
			std::string host,
			std::string port,
			std::string resource
		);

		/// \brief true if client is connected to server, false otherwise
		bool is_connected()const;


		/// \brief Send a text message
		void send_text(shared_const_buffer buffer);

		/// \brief Send a binary message
		void send_binary(shared_const_buffer buffer);


		/// \brief Close the sessions
		void close(boost::beast::string_view reason);


		/// \brief Wait on the processing threads
		///
		/// This effecivly blocks the current thread until the client is closed.
		void block()noexcept;

		/// \brief Close the connection
		///
		/// This function is not blocking. Call block() if you want to wait
		/// until all connections are closed.
		void shutdown()noexcept;


		/// \brief Get executor
		boost::asio::io_context::executor_type get_executor();

		/// \brief Run one task in client thread
		std::size_t poll_one()noexcept;


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
		virtual void on_text(boost::beast::multi_buffer&& buffer);

		/// \brief Called when the session received a binary message
		///
		/// Default implementation does nothing.
		virtual void on_binary(boost::beast::multi_buffer&& buffer);

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


		/// \brief Set max size of incomming WebSocket messages
		void set_max_read_message_size(std::size_t bytes){
			max_read_message_size_ = bytes;
		}

		/// \brief Max size of incomming WebSocket messages
		std::size_t max_read_message_size()const{
			return max_read_message_size_;
		}


		/// \brief Set session timeout
		void set_ping_time(std::chrono::milliseconds ms){
			ping_time_ = ms;
		}

		/// \brief Session timeout
		std::chrono::milliseconds ping_time()const{
			return ping_time_;
		}


		/// \brief Called by the connection when it closes
		void remove_session();


	private:
		/// \brief Max size of incomming http and WebSocket messages
		std::size_t max_read_message_size_{16 * 1024 * 1024};

		/// \brief WebSocket session timeout
		///
		/// After this time without an incomming message a ping is send.
		/// If no message is incomming after a second period of this time, the
		/// session is considerd to be dead and will be closed.
		std::chrono::milliseconds ping_time_{15000};


		/// \brief Pointer to the current session
		std::unique_ptr< class ws_client_session > session_;


		/// \brief The clients io_context
		boost::asio::io_context ioc_;

		/// \brief Operations are done async over this stand
		strand strand_;

		/// \brief Keeps the io_context running
		boost::asio::executor_work_guard<
			boost::asio::io_context::executor_type > work_;


		/// \brief Shutdown flag
		std::atomic< std::size_t > shutdown_{false};

		/// \brief Count of currently running async calls
		std::atomic< std::size_t > async_calls_{0};


		/// \brief Protect thread joins
		std::mutex mutex_;

		/// \brief The working thread
		std::thread thread_;
	};


}


#endif
