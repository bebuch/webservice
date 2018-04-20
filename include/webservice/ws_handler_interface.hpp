//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__ws_handler_interface__hpp_INCLUDED_
#define _webservice__ws_handler_interface__hpp_INCLUDED_

#include "ws_identifier.hpp"

#include <boost/asio/ip/tcp.hpp>

#include <boost/beast/http.hpp>


namespace webservice{


	using http_request
		= boost::beast::http::request< boost::beast::http::string_body >;


	/// \brief The interface for creating server websocket sessions
	class ws_handler_interface{
	public:
		/// \brief Constructor
		ws_handler_interface() = default;


		/// \brief Destructor
		virtual ~ws_handler_interface() = default;


		ws_handler_interface(ws_handler_interface const&) = delete;

		ws_handler_interface& operator=(ws_handler_interface const&) = delete;


		/// \brief Set the owning server/client
		///
		/// Called by the server/client.
		void set_executor(class executor& executor)noexcept;

		/// \brief Create a new server websocket session
		///
		/// Called by the server.
		void server_connect(
			boost::asio::ip::tcp::socket&& socket,
			http_request&& req);

		/// \brief Create a new client websocket session
		///
		/// Called by the client.
		void client_connect(
			std::string host,
			std::string port,
			std::string resource);

		/// \brief Server is shutting down
		///
		/// Called by the server/client.
		void shutdown()noexcept;


		/// \brief Get reference to server/client executor
		class executor& executor();


		/// \brief Called when an exception was thrown
		///
		/// Default implementation does nothing.
		virtual void on_exception(std::exception_ptr error)noexcept;


	private:
		/// \brief Called by set_executor
		virtual void on_executor() = 0;

		/// \brief Create a new server websocket connection
		virtual void on_server_connect(
			boost::asio::ip::tcp::socket&& socket,
			http_request&& req);

		/// \brief Create a new client websocket session
		virtual void on_client_connect(
			std::string&& host,
			std::string&& port,
			std::string&& resource);

		/// \brief Shutdown hint called by shutdown()
		virtual void on_shutdown()noexcept = 0;


		/// \brief Pointer to the executor object
		class executor* executor_ = nullptr;
	};


}


#endif
