//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webserver
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webserver__websocket_session__hpp_INCLUDED_
#define _webserver__websocket_session__hpp_INCLUDED_

#include <webserver/fail.hpp>

#include <boost/beast/websocket.hpp>

#include <boost/asio/strand.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/bind_executor.hpp>


namespace webserver{


	// Echoes back all received WebSocket messages
	class websocket_session
		: public std::enable_shared_from_this< websocket_session >
	{
	public:
		// Take ownership of the socket
		explicit websocket_session(boost::asio::ip::tcp::socket socket)
			: ws_(std::move(socket))
			, strand_(ws_.get_executor())
			, timer_(ws_.get_executor().context(),
				std::chrono::steady_clock::time_point::max()) {}

		// Start the asynchronous operation
		template < typename Body, typename Allocator >
		void do_accept(
			boost::beast::http::request<
				Body, boost::beast::http::basic_fields< Allocator > > req
		){
			// Set the control callback. This will be called
			// on every incoming ping, pong, and close frame.
			ws_.control_callback(
				[this](
					boost::beast::websocket::frame_type /*kind*/,
					boost::beast::string_view /*payload*/
				){
					// Note that there is activity
					activity();
				});


			// Run the timer. The timer is operated
			// continuously, this simplifies the code.
			on_timer({});

			// Set the timer
			using namespace std::literals::chrono_literals;
			timer_.expires_after(15s);

			// Accept the websocket handshake
			ws_.async_accept(
				req,
				boost::asio::bind_executor(
					strand_,
					[this_ = shared_from_this()](boost::system::error_code ec){
						this_->on_accept(ec);
					}));
		}

		void on_accept(boost::system::error_code ec){
			// Happens when the timer closes the socket
			if(ec == boost::asio::error::operation_aborted){
				return;
			}

			if(ec){
				return log_fail(ec, "accept");
			}

			// Read a message
			do_read();
		}

		// Called when the timer expires.
		void on_timer(boost::system::error_code ec){
			if(ec && ec != boost::asio::error::operation_aborted){
				return log_fail(ec, "timer");
			}

			// See if the timer really expired since the deadline may have
			// moved.
			if(timer_.expiry() <= std::chrono::steady_clock::now()){
				// If this is the first time the timer expired,
				// send a ping to see if the other end is there.
				if(ws_.is_open() && ping_state_ == 0){
					// Note that we are sending a ping
					ping_state_ = 1;

					// Set the timer
					using namespace std::literals::chrono_literals;
					timer_.expires_after(15s);

					// Now send the ping
					ws_.async_ping({},
						boost::asio::bind_executor(
							strand_,
							[this_ = shared_from_this()](
								boost::system::error_code ec
							){
								this_->on_ping(ec);
							}));
				}else{
					// The timer expired while trying to handshake,
					// or we sent a ping and it never completed or
					// we never got back a control frame, so close.

					// Closing the socket cancels all outstanding operations.
					// They will complete with
					// boost::asio::error::operation_aborted
					ws_.next_layer().shutdown(
						boost::asio::ip::tcp::socket::shutdown_both, ec);
					ws_.next_layer().close(ec);
					return;
				}
			}

			// Wait on the timer
			timer_.async_wait(
				boost::asio::bind_executor(
					strand_,
					[this_ = shared_from_this()](boost::system::error_code ec){
						this_->on_timer(ec);
					}));
		}

		// Called to indicate activity from the remote peer
		void activity() {
			// Note that the connection is alive
			ping_state_ = 0;

			// Set the timer
			timer_.expires_after(std::chrono::seconds(15));
		}

		// Called after a ping is sent.
		void on_ping(boost::system::error_code ec){
			// Happens when the timer closes the socket
			if(ec == boost::asio::error::operation_aborted){
				return;
			}

			if(ec){
				return log_fail(ec, "ping");
			}

			// Note that the ping was sent.
			if(ping_state_ == 1){
				ping_state_ = 2;
			}else{
				// ping_state_ could have been set to 0
				// if an incoming control frame was received
				// at exactly the same time we sent a ping.
				BOOST_ASSERT(ping_state_ == 0);
			}
		}

		void do_read(){
			// Set the timer
			timer_.expires_after(std::chrono::seconds(15));

			// Read a message into our buffer
			ws_.async_read(
				buffer_,
				boost::asio::bind_executor(
					strand_,
					[this_ = shared_from_this()](
						boost::system::error_code ec,
						std::size_t bytes_transferred
					){
						this_->on_read(ec, bytes_transferred);
					}));
		}

		void on_read(
			boost::system::error_code ec,
			std::size_t bytes_transferred
		){
			boost::ignore_unused(bytes_transferred);

			// Happens when the timer closes the socket
			if(ec == boost::asio::error::operation_aborted){
				return;
			}

			// This indicates that the websocket_session was closed
			if(ec == boost::beast::websocket::error::closed){
				return;
			}

			if(ec){
				log_fail(ec, "read");
			}

			// Note that there is activity
			activity();

			// Echo the message
			ws_.text(ws_.got_text());
			ws_.async_write(
				buffer_.data(),
				boost::asio::bind_executor(
					strand_,
					[this_ = shared_from_this()](
						boost::system::error_code ec,
						std::size_t bytes_transferred
					){
						this_->on_write(ec, bytes_transferred);
					}));
		}

		void on_write(
			boost::system::error_code ec,
			std::size_t bytes_transferred
		){
			boost::ignore_unused(bytes_transferred);

			// Happens when the timer closes the socket
			if(ec == boost::asio::error::operation_aborted){
				return;
			}

			if(ec){
				return log_fail(ec, "write");
			}

			// Clear the buffer
			buffer_.consume(buffer_.size());

			// Do another read
			do_read();
		}

		/// \brief Send a text message to all sessions
		void send_text(std::string&& data);

		/// \brief Send a binary message to all sessions
		void send_binary(std::vector< std::uint8_t >&& data);


	private:
		boost::beast::websocket::stream< boost::asio::ip::tcp::socket > ws_;
		boost::asio::strand< boost::asio::io_context::executor_type > strand_;
		boost::asio::steady_timer timer_;
		boost::beast::multi_buffer buffer_;
		char ping_state_ = 0;
	};


}


#endif
