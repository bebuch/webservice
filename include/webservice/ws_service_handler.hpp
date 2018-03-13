//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__ws_service_handler__hpp_INCLUDED_
#define _webservice__ws_service_handler__hpp_INCLUDED_

#include "ws_handler_base.hpp"

#include <memory>
#include <set>


namespace webservice{


	class ws_service_handler: public ws_handler_base{
	public:
		/// \brief Constructor
		ws_service_handler();

		/// \brief Destructor
		~ws_service_handler()override;


		/// \brief Add ws_handler_base that is used for sessions with resource
		///        name
		///
		/// \throw std::logic_error if a ws_handler_base with same name did
		///                         already exist
		///
		/// Thread safe: Yes.
		void add_service(
			std::string name,
			std::unique_ptr< class ws_handler_base > service);

		/// \brief Erase ws_handler_base with name
		///
		/// \throw std::logic_error if no ws_handler_base with name did exist
		///
		/// Thread safe: Yes.
		void erase_service(std::string name);


	protected:
		/// \brief Rebinds the session to a service
		void on_open(
			ws_server_session* session,
			std::string const& resource)override;

		/// \brief Called if there was no service with resource as name
		///
		/// Closes the session.
		virtual void on_unknown_service(
			ws_server_session* session,
			std::string const& resource);


	private:
		/// \brief Pointer to implementation
		std::unique_ptr< struct ws_service_handler_impl > impl_;
	};


}


#endif
