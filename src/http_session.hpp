//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__http_session__hpp_INCLUDED_
#define _webservice__http_session__hpp_INCLUDED_

#include <webservice/http_request_handler.hpp>

#include <boost/circular_buffer.hpp>

#include <boost/beast/core/flat_buffer.hpp>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/steady_timer.hpp>

#include <mutex>


namespace webservice{


	class http_request_handler;

	/// \brief Handles an HTTP server session
	class http_session{
	public:
		/// \brief Take ownership of the socket and start reading
		explicit http_session(
			boost::asio::ip::tcp::socket&& socket,
			http_request_handler& handler
		);

		~http_session();

		// Called when the timer expires.
		void do_timer();

		void run();

		void do_read();

		void on_write(boost::system::error_code ec, bool close);

		/// \brief Send a TCP shutdown
		void do_close();

		/// \brief Called by the HTTP handler to send a response.
		void response(std::unique_ptr< http_session_work >&& work);

		/// \brief Send a request to erase this session from the list
		///
		/// The request is sended only once, any call after the fist will be
		/// ignored.
		void async_erase();


	private:
		// This queue is used for HTTP pipelining.
		class queue{
			/// \brief Maximum number of responses we will queue
			static constexpr std::size_t limit = 64;

		public:
			static_assert(limit > 0, "queue limit must be positive");

			explicit queue();

			// Returns `true` if we have reached the queue limit
			bool is_full()const;

			// Called when a message finishes sending
			// Returns `true` if the caller should initiate a read
			bool on_write();

			// Called by the HTTP handler to send a response.
			void response(std::unique_ptr< http_session_work >&& work);


		private:
			boost::circular_buffer< std::unique_ptr< http_session_work > >
				items_;
		};


		http_request_handler& handler_;

		boost::asio::ip::tcp::socket socket_;
		boost::asio::strand< boost::asio::io_context::executor_type > strand_;
		boost::asio::steady_timer timer_;
		boost::beast::flat_buffer buffer_;

		http_request req_;
		queue queue_;

		std::once_flag erase_flag_;
		std::atomic< std::size_t > async_calls_{0};
	};


}


#endif
