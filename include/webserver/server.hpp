//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webserver
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webserver__server__hpp_INCLUDED_
#define _webserver__server__hpp_INCLUDED_

#include "http_request_handler.hpp"


namespace webserver{


	class server_impl;

	/// \brief An http and WebSocket server
	class server{
	public:
		/// \brief Type of an exception handling function
		using exception_handler = std::function< void(std::exception_ptr) >;


		/// \brief Constructor
		///
		/// \param handler Handles the sessions
		/// \param address IP address (IPv4 or IPv6)
		/// \param port TCP Port
		/// \param thread_count Count of threads that proccess request parallel
		/// \param handle_exception Is called if an asynchronous operation
		///                         returns by exception. If you use more than
		///                         one server thread, then your handling
		///                         function must be thread save!
		server(
			http_request_handler& handler,
			boost::asio::ip::address address,
			std::uint16_t port,
			std::uint8_t thread_count = 1,
			exception_handler handle_exception = {}
		);

		/// \brief Close all connections and wait on all processing threads
		///
		/// Calls close() and block()
		~server();

		/// \brief Wait on all processing threads
		///
		/// This effecivly blocks the current thread until the server is closed.
		void block();

		/// \brief Close all connections as fast as possible
		///
		/// This function is not blocking. Call block() if you want to wait
		/// until all connections are closed.
		void stop();


	private:
		/// \brief Pointer to implementation
		std::unique_ptr< server_impl > impl_;
	};


}


#endif
