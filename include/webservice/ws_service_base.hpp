//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__ws_service_base__hpp_INCLUDED_
#define _webservice__ws_service_base__hpp_INCLUDED_

#include "shared_const_buffer.hpp"
#include "ws_handler_interface.hpp"
#include "ws_service_interface.hpp"
#include "ws_session_settings.hpp"
#include "ws_session_container.hpp"

#include <boost/beast/core/string.hpp>

#include <string>
#include <set>


namespace webservice{


	/// \brief Base for any server websocket service that handels standing
	///        sessions
	template < typename Value >
	class ws_service_base
		: ws_handler_interface
		, ws_service_interface
		, ws_session_settings{
	public:
		/// \brief Constructor
		ws_service_base();

		/// \brief Destructor
		~ws_service_base()override;


		ws_service_base(ws_service_base const&) = delete;

		ws_service_base& operator=(ws_service_base const&) = delete;


		/// \brief Emplace a new ws_handler_server_session
		void on_make(
			boost::asio::ip::tcp::socket&& socket,
			http_request&& req)override;

		/// \brief Create the implementation
		void on_server(class server& server)override;

		/// \brief Accept no new sessions, send close to all session
		void on_shutdown()noexcept override;


		/// \brief Called by a ws_handler_server_session
		void async_erase(class ws_handler_server_session* session);


		/// \brief Send a text message to session
		void send_text(
			ws_identifier identifier,
			shared_const_buffer buffer);

		/// \brief Send a text message to sessions
		void send_text(
			std::set< ws_identifier > const& identifiers,
			shared_const_buffer buffer);

		/// \brief Send a text message to all session
		void send_text(shared_const_buffer buffer);


		/// \brief Send a binary message to session
		void send_binary(
			ws_identifier identifier,
			shared_const_buffer buffer);

		/// \brief Send a binary message to sessions
		void send_binary(
			std::set< ws_identifier > const& identifiers,
			shared_const_buffer buffer);

		/// \brief Send a binary message to all session
		void send_binary(shared_const_buffer buffer);


		/// \brief Shutdown session
		void close(
			ws_identifier identifier,
			boost::beast::string_view reason);

		/// \brief Shutdown sessions
		void close(
			std::set< ws_identifier > const& identifiers,
			boost::beast::string_view reason);

		/// \brief Shutdown all sessions
		void close(boost::beast::string_view reason);


		/// \brief true if server is shutting down
		bool is_shutdown()noexcept;


	private:
		/// \brief Pointer to implementation
		std::unique_ptr< ws_session_container< Value > > list_;
	};


}


#endif
