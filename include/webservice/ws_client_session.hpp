//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__ws_client_session__hpp_INCLUDED_
#define _webservice__ws_client_session__hpp_INCLUDED_

#include "ws_session.hpp"


namespace webservice{


	class ws_client_base;

	class ws_client_session: public ws_session< ws_client_session >{
	public:
		/// \brief Take ownership of the socket
		explicit ws_client_session(
			ws_stream&& ws,
			ws_client_base& client,
			std::chrono::milliseconds ping_time);

		/// \brief Destructor
		~ws_client_session();


		/// \brief Start the session
		void start();


		/// \brief Called when the sessions start
		void on_open()noexcept;

		/// \brief Called when the sessions ends
		void on_close()noexcept;

		/// \brief Called when the session received a text message
		void on_text(boost::beast::multi_buffer&& buffer)noexcept;

		/// \brief Called when the session received a binary message
		void on_binary(boost::beast::multi_buffer&& buffer)noexcept;

		/// \brief Called when an error occured
		void on_error(
			boost::beast::string_view location,
			boost::system::error_code ec)noexcept;

		/// \brief Called when an exception was thrown
		void on_exception(std::exception_ptr error)noexcept;


		/// \brief Remove session on client
		void remove()noexcept;


	private:
		ws_client_base& client_;
		bool is_open_ = false;
	};


}


#endif
