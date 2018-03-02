//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__http_request_handler__hpp_INCLUDED_
#define _webservice__http_request_handler__hpp_INCLUDED_

#include "http_request_location.hpp"
#include "http_response.hpp"

#include <boost/beast/http.hpp>


namespace webservice{


	using http_request
		= boost::beast::http::request< boost::beast::http::string_body >;

	using http_string_response
		= boost::beast::http::response< boost::beast::http::string_body >;


	/// \brief Base class of all HTTP request handlers
	class http_request_handler{
	public:
		http_request_handler() = default;

		http_request_handler(http_request_handler const&) = delete;

		http_request_handler& operator=(http_request_handler const&) = delete;


		virtual ~http_request_handler();
		virtual void operator()(http_request&& req, http_response&& send);

	protected:
		/// \brief Called when an error occured
		///
		/// Default implementation does nothing.
		virtual void on_error(
			http_request_location location,
			boost::system::error_code ec);

		/// \brief Called when an exception occurred
		///
		/// Default implementation does nothing.
		virtual void on_exception(std::exception_ptr error)noexcept;


		/// \brief Get reference to const server
		///
		/// Must not be called before a server is initialized with this service.
		class server const& server()const noexcept{
			assert(server_ != nullptr);
			return *server_;
		}

		/// \brief Get reference to server
		///
		/// Must not be called before a server is initialized with this service.
		class server& server()noexcept{
			assert(server_ != nullptr);
			return *server_;
		}


	private:
		/// \brief Pointer to the server object
		class server* server_ = nullptr;

		friend class http_session;
		friend class server;
	};

	/// \brief Returns a bad request response
	http_string_response bad_request(
		http_request const& req,
		boost::beast::string_view why
	);

	/// \brief Returns a not found response
	http_string_response not_found(
		http_request const& req,
		boost::beast::string_view target
	);

	/// \brief Returns a server error response
	http_string_response server_error(
		http_request const& req,
		boost::beast::string_view what
	);


}


#endif
