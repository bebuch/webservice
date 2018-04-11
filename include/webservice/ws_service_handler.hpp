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


		/// \brief Create a new ws_server_session
		void async_emplace(
			boost::asio::ip::tcp::socket&& socket,
			http_request&& req)override;


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


		/// \brief Call shutdown on all services and don't accept any new ones
		void on_shutdown()noexcept override;

		/// \brief Set the owning server
		void set_server(class server& server)override;


	private:
		/// \brief Count of running async operations
		std::atomic< std::size_t > async_calls_{0};

		/// \brief Pointer to implementation
		std::unique_ptr< struct ws_service_handler_impl > impl_;
	};


}


#endif
