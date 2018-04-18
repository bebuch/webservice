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


		/// \brief Set the owning server
		///
		/// Called by the server.
		void set_server(class server& server)noexcept;

		/// \brief Make a new websocket session
		///
		/// Called by the server.
		void make(boost::asio::ip::tcp::socket&& socket, http_request&& req);

		/// \brief Server is shutting down
		///
		/// Called by the server.
		void shutdown()noexcept;


		/// \brief Get reference to server
		class server* server()noexcept;


	private:
		/// \brief Called by set_server
		virtual void on_server(class server& server) = 0;

		/// \brief Create a new ws_server_session
		virtual void on_make(
			boost::asio::ip::tcp::socket&& socket,
			http_request&& req) = 0;

		/// \brief Shutdown hint called by shutdown()
		virtual void on_shutdown()noexcept = 0;


		/// \brief Pointer to the server object
		class server* server_ = nullptr;
	};


}


#endif
