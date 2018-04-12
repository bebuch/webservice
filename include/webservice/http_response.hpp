//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__http_response__hpp_INCLUDED_
#define _webservice__http_response__hpp_INCLUDED_

#include "async_lock.hpp"

#include <boost/beast/http/message.hpp>
#include <boost/beast/http/write.hpp>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/bind_executor.hpp>

#include <memory>


namespace webservice{


	class http_session;

	class http_session_on_write{
	public:
		http_session_on_write(
			class http_session* self,
			std::atomic< std::size_t >& async_calls,
			bool const need_eof
		)
			: self_(self)
			, async_lock_(async_calls, "http_session_on_write")
			, need_eof_(need_eof) {}

		void operator()(
			boost::system::error_code ec,
			std::size_t /*bytes_transferred*/
		)const;

	private:
		class http_session* self_;
		async_lock async_lock_;
		bool const need_eof_;
	};

	// The type-erased, saved work item
	struct http_session_work{
		virtual ~http_session_work() = default;
		virtual void operator()() = 0;
	};

	class http_response{
	public:
		using response_fn =
			void (http_session::*)(std::unique_ptr< http_session_work >&&);

		http_response(
			class http_session* self,
			std::atomic< std::size_t >& async_calls,
			response_fn fn,
			boost::asio::ip::tcp::socket& socket,
			boost::asio::strand<
				boost::asio::io_context::executor_type >& strand
		)
			: self_(self)
			, async_calls_(async_calls)
			, fn_(fn)
			, socket_(socket)
			, strand_(strand) {}

		// Called by the HTTP handler to send a response.
		template < typename Body, typename Fields >
		void operator()(
			boost::beast::http::response< Body, Fields >&& msg
		){
			// This holds a work item
			class work_impl: public http_session_work{
			public:
				work_impl(
					class http_session* self,
					std::atomic< std::size_t >& async_calls,
					boost::asio::ip::tcp::socket& socket,
					boost::asio::strand<
						boost::asio::io_context::executor_type >& strand,
					boost::beast::http::response< Body, Fields >&& msg
				)
					: self_(self)
					, async_calls_(async_calls)
					, socket_(socket)
					, strand_(strand)
					, msg_(std::move(msg)) {}

				void operator()(){
					boost::beast::http::async_write(
						socket_,
						msg_,
						boost::asio::bind_executor(
							strand_,
							http_session_on_write(
								self_, async_calls_, msg_.need_eof())
						));
				}

			private:
				class http_session* self_;
				std::atomic< std::size_t >& async_calls_;
				boost::asio::ip::tcp::socket& socket_;
				boost::asio::strand<
					boost::asio::io_context::executor_type >& strand_;
				boost::beast::http::response< Body, Fields > msg_;
			};

			((*self_).*fn_)(std::make_unique< work_impl >(
				self_, async_calls_, socket_, strand_, std::move(msg)));
		}

	private:
		class http_session* self_;
		std::atomic< std::size_t >& async_calls_;
		response_fn fn_;
		boost::asio::ip::tcp::socket& socket_;
		boost::asio::strand< boost::asio::io_context::executor_type >& strand_;
	};


}


#endif
