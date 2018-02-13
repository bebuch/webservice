//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include "websocket_service_impl.hpp"

#include <webservice/websocket_service.hpp>
#include <webservice/fail.hpp>

#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/websocket.hpp>

#include <boost/asio/strand.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/bind_executor.hpp>


namespace webservice{


	websocket_session::websocket_session(
		boost::asio::ip::tcp::socket socket,
		websocket_service& service
	)
		: service_(service)
		, ws_(std::move(socket))
		, strand_(ws_.get_executor())
		, timer_(ws_.get_executor().context(),
			std::chrono::steady_clock::time_point::max()) {}

	websocket_session::~websocket_session(){
		if(is_open_){
			try{
				service_.impl_->on_close(this);
			}catch(...){
				log_exception(std::current_exception(),
					"websocket_service::on_close");
			}
		}
	}

	void websocket_session::do_accept(
		boost::beast::http::request< boost::beast::http::string_body > req
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

	void websocket_session::on_accept(boost::system::error_code ec){
		// Happens when the timer closes the socket
		if(ec == boost::asio::error::operation_aborted){
			return;
		}

		if(ec){
			return log_fail(ec, "accept");
		}

		is_open_ = true;
		try{
			service_.impl_->on_open(this);
		}catch(...){
			log_exception(std::current_exception(),
				"websocket_service::on_open");
		}

		// Read a message
		do_read();
	}

	void websocket_session::on_timer(boost::system::error_code ec){
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

	void websocket_session::activity() {
		// Note that the connection is alive
		ping_state_ = 0;

		// Set the timer
		timer_.expires_after(std::chrono::seconds(15));
	}

	void websocket_session::on_ping(boost::system::error_code ec){
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

	void websocket_session::do_read(){
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

	void websocket_session::on_read(
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
		if(ws_.got_text()){
			try{
				service_.impl_->on_text(this, buffer_);
			}catch(...){
				log_exception(std::current_exception(),
					"websocket_service::on_text");
			}
		}else{
			try{
				service_.impl_->on_binary(this, buffer_);
			}catch(...){
				log_exception(std::current_exception(),
					"websocket_service::on_binary");
			}
		}

		// Clear the buffer
		buffer_.consume(buffer_.size());

		// Do another read
		do_read();
	}

	void websocket_session::on_write(
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
	}


}
