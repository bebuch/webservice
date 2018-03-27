//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__server_impl__hpp_INCLUDED_
#define _webservice__server_impl__hpp_INCLUDED_

#include "http_request_handler.hpp"
#include "ws_handler_base.hpp"
#include "error_handler.hpp"

#include <boost/asio/executor.hpp>


namespace webservice{


	/// \brief An http and WebSocket server_impl
	class server_impl{
	public:
		/// \brief Constructor
		///
		/// \param http_handler Handles HTTP sessions
		/// \param service Handles ws sessions
		/// \param error_handler Handles error in the server_impl
		/// \param address IP address (IPv4 or IPv6)
		/// \param port TCP Port
		server_impl(
			std::unique_ptr< http_request_handler >&& http_handler,
			std::unique_ptr< ws_handler_base >&& service,
			std::unique_ptr< error_handler >&& error_handler,
			boost::asio::ip::address address,
			std::uint16_t port,
			std::uint8_t thread_count
		);

		server_impl(server_impl const&) = delete;

		server_impl& operator=(server_impl const&) = delete;

		/// \brief Close all connections and wait on all processing threads
		///
		/// Calls close() and block()
		~server_impl();

		/// \brief Wait on all processing threads
		///
		/// This effecivly blocks the current thread until the server_impl is
		/// closed.
		void block()noexcept;

		/// \brief Cancal all operations
		///
		/// This function is not blocking. Call block() if you want to wait
		/// until all operations are canceled.
		///
		/// This function doesn't close the connections properly! Use
		/// shutdown() for this.
		void stop()noexcept;

		/// \brief Don't accept new connections and async tasks
		///
		/// This function is not blocking. Call block() if you want to wait
		/// until all connections are closed.
		void shutdown()noexcept;

		/// \brief Get executor
		boost::asio::executor get_executor();


		/// \brief Run one task in server_impl threads
		std::size_t run_one()noexcept;


		/// \brief true if a WebSocket handler is set, false otherwise
		bool has_ws()const{
			return service_.get() != nullptr;
		}


		/// \brief Reference to the error_handler
		error_handler& error()const{
			return *error_handler_;
		}

		/// \brief Reference to the http handler
		http_request_handler& http()const{
			return *handler_;
		}

		/// \brief Reference to the WebSocket handler
		///
		/// \pre has_ws() must be true
		ws_handler_base& ws()const{
			assert(service_.get() != nullptr);
			return *service_;
		}


	private:
		/// \brief Handler for HTTP sessions
		std::unique_ptr< http_request_handler > handler_;

		/// \brief Handler for WebSocket sessions
		std::unique_ptr< ws_handler_base > service_;

		/// \brief Handles errors and exceptions in the server
		std::unique_ptr< error_handler > error_handler_;

		/// \brief Protect thread joins
		std::recursive_mutex mutex_;

		/// \brief The worker threads
		std::vector< std::thread > threads_;

		/// \brief The io_context is required for all I/O
		boost::asio::io_context ioc_;

		/// \brief Accepts incoming connections and launches the sessions
		listener listener_;
	};


}


#endif
