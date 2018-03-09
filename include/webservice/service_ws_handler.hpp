//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__service_ws_handler__hpp_INCLUDED_
#define _webservice__service_ws_handler__hpp_INCLUDED_

#include "ws_handler_base.hpp"

#include <memory>
#include <set>


namespace webservice{


	class service_ws_handler: public ws_handler_base{
	public:
		/// \brief Constructor
		service_ws_handler();

		/// \brief Destructor
		~service_ws_handler()override;


		/// \brief Add ws_service_base that is used for sessions with resource
		///        name
		///
		/// \throw std::logic_error if a ws_service_base with same name did
		///                         already exist
		///
		/// Thread safe: Yes.
		void add_service(
			std::string name,
			std::unique_ptr< class ws_service_base > service);

		/// \brief Erase ws_service_base with name
		///
		/// \throw std::logic_error if no ws_service_base with name did exist
		///
		/// Thread safe: Yes.
		void erase_service(std::string name);


	protected:
		/// \brief Called with a unique identifier when a sessions starts
		///
		/// Default implementation does nothing.
		void on_open(
			ws_server_session* session,
			std::string const& resource);

		/// \brief Called with a unique identifier when a sessions ends
		///
		/// Default implementation does nothing.
		void on_close(
			ws_server_session* session,
			std::string const& resource);

		/// \brief Called when a session received a text message
		///
		/// Default implementation does nothing.
		void on_text(
			ws_server_session* session,
			std::string const& resource,
			boost::beast::multi_buffer const& buffer);

		/// \brief Called when a session received a binary message
		///
		/// Default implementation does nothing.
		void on_binary(
			ws_server_session* session,
			std::string const& resource,
			boost::beast::multi_buffer const& buffer);

		/// \brief Called when an error occured
		///
		/// Default implementation does nothing.
		void on_error(
			ws_server_session* session,
			std::string const& resource,
			ws_handler_location location,
			boost::system::error_code ec);

		/// \brief Called when an exception was thrown
		///
		/// Default implementation does nothing.
		void on_exception(
			ws_server_session* session,
			std::string const& resource,
			std::exception_ptr error)noexcept;


	private:
		/// \brief Pointer to implementation
		std::unique_ptr< class service_ws_handler_impl > impl_;
	};


}


#endif
