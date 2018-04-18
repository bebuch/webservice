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


	/// \brief Refers sessions to sub-service by the requested target name
	final class ws_service_handler: public ws_handler_interface{
	public:
		/// \brief Constructor
		ws_service_handler();

		/// \brief Destructor
		///
		/// Block until last async operation has finished.
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
		/// \attention A service must not remove itself!
		///
		/// \throw std::logic_error if no ws_handler_base with name did exist
		///
		/// Thread safe: Yes.
		void erase_service(std::string name);


		/// \brief true if on_shutdown was called
		bool is_shutdown()noexcept{
			return !run_lock_.is_locked();
		}


	private:
		/// \brief Create the service map
		void on_server(class server& server)override;

		/// \brief Create a new ws_server_session
		void on_make(
			boost::asio::ip::tcp::socket&& socket,
			http_request&& req)override;

		/// \brief Call shutdown on all services
		void on_shutdown()noexcept override;


		/// \brief Pointer to implementation
		std::unique_ptr< struct ws_service_handler_impl > impl_;
	};


}


#endif
