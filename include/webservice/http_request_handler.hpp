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


	template < typename T >
	class sessions;


	/// \brief Base class of all HTTP request handlers
	class http_request_handler{
	public:
		/// \brief Constructor
		http_request_handler();

		http_request_handler(http_request_handler const&) = delete;

		http_request_handler& operator=(http_request_handler const&) = delete;


		/// \brief Destructor
		virtual ~http_request_handler();


		/// \brief Create a new http_session
		void emplace(boost::asio::ip::tcp::socket&& socket);

		/// \brief Process http request
		virtual void operator()(http_request&& req, http_response&& send);


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


		/// \brief Set the owning server
		virtual void set_server(class server* server);


		std::chrono::milliseconds timeout()noexcept{
			return std::chrono::milliseconds(15000);
		}

	protected:
		/// \brief Get reference to server
		class server* server()noexcept;


	private:
		/// \brief Pointer to implementation
		std::unique_ptr< sessions< class http_session > > list_;
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
