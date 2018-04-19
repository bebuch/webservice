//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__ws_server_session__hpp_INCLUDED_
#define _webservice__ws_server_session__hpp_INCLUDED_

#include "ws_session.hpp"


namespace webservice{


	class ws_service_interface;

	class ws_server_session: public ws_session< ws_server_session >{
	public:
		/// \brief Take ownership of the socket
		explicit ws_server_session(
			ws_stream&& ws,
			ws_service_interface& service,
			std::chrono::milliseconds ping_time);

		/// \brief Destructor
		~ws_server_session();


		/// \brief Start the asynchronous operation
		void do_accept(http_request&& req);


		/// \brief Called with when a sessions starts
		void on_open()noexcept;

		/// \brief Called with when a sessions ends
		void on_close()noexcept;

		/// \brief Called when a text message
		void on_text(boost::beast::multi_buffer&& buffer)noexcept;

		/// \brief Called when a binary message
		void on_binary(boost::beast::multi_buffer&& buffer)noexcept;

		/// \brief Called when an error occured
		void on_error(
			boost::beast::string_view location,
			boost::system::error_code ec)noexcept;

		/// \brief Called when an exception was thrown
		void on_exception(std::exception_ptr error)noexcept;


		/// \brief Remove session from handler list
		void remove()noexcept;


	private:
		ws_service_interface& service_;

		bool is_open_ = false;
	};


}


#endif
