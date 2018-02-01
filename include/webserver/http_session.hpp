//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webserver
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webserver__http_session__hpp_INCLUDED_
#define _webserver__http_session__hpp_INCLUDED_

#include <boost/asio/ip/tcp.hpp>

#include <boost/system/error_code.hpp>

#include <memory>
#include <string>


namespace webserver{


	class http_session_impl;

	/// \brief Handles an HTTP server connection
	class http_session: public std::enable_shared_from_this< http_session >{
	public:
		/// \brief Take ownership of the socket
		explicit http_session(
			boost::asio::ip::tcp::socket socket,
			std::string const& doc_root);

		/// \brief Destructor
		~http_session();


		/// \brief Start the asynchronous operation
		void run();


		/// \brief Reference to implementation
		///
		/// This is an implementation detail and not part of the interface.
		/// It can only be used in http_session.cpp because http_session_impl
		/// is only defined there.
		inline http_session_impl& impl();


	private:
		/// \brief Implementation pointer
		std::unique_ptr< http_session_impl > impl_;
	};


}


#endif
