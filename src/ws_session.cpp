//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include "ws_service_impl.hpp"
#include "ws_client_impl.hpp"

#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/websocket.hpp>

#include <boost/asio/strand.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/dispatch.hpp>


namespace webservice{


	template < typename Derived >
	ws_session< Derived >::ws_session(ws_stream&& ws)
		: ws_(std::move(ws))
		, strand_(ws_.get_executor())
		, timer_(ws_.get_executor().context(),
			std::chrono::steady_clock::time_point::max()) {}

	template < typename Derived >
	void ws_session< Derived >::on_timer(boost::system::error_code ec){
		if(ec && ec != boost::asio::error::operation_aborted){
			this->callback::on_error(error_type::timer, ec);
			return;
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
				start_timer();

				// Now send the ping
				ws_.async_ping({},
					boost::asio::bind_executor(
						strand_,
						[this_ = this->shared_from_this()](
							boost::system::error_code ec
						){
							this_->on_ping(ec);
						}));
			}else{
				close(ec);
				return;
			}
		}

		// Wait on the timer
		if(ws_.is_open()){
			timer_.async_wait(
				boost::asio::bind_executor(
					strand_,
					[this_ = this->shared_from_this()]
					(boost::system::error_code ec){
						this_->on_timer(ec);
					}));
		}
	}

	template < typename Derived >
	void ws_session< Derived >::start_timer(){
		timer_.expires_after(std::chrono::seconds(15));
	}

	template < typename Derived >
	void ws_session< Derived >::activity() {
		// Note that the connection is alive
		ping_state_ = 0;

		// Set the timer
		start_timer();
	}

	template < typename Derived >
	void ws_session< Derived >::on_ping(boost::system::error_code ec){
		// Happens when the timer closes the socket
		if(ec == boost::asio::error::operation_aborted){
			return;
		}

		if(ec){
			this->callback::on_error(error_type::ping, ec);
			return;
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

	template < typename Derived >
	void ws_session< Derived >::do_read(){
		start_timer();

		// Read a message into our buffer
		if(ws_.is_open()){
			ws_.async_read(
				buffer_,
				boost::asio::bind_executor(
					strand_,
					[this_ = this->shared_from_this()](
						boost::system::error_code ec,
						std::size_t /*bytes_transferred*/
					){
						this_->on_read(ec);
					}));
		}
	}

	template < typename Derived >
	void ws_session< Derived >::on_read(boost::system::error_code ec){
		// Happens when the timer closes the socket
		if(ec == boost::asio::error::operation_aborted){
			return;
		}

		// This indicates that the ws_session was closed
		if(ec == boost::beast::websocket::error::closed){
			return;
		}

		if(ec){
			this->callback::on_error(error_type::read, ec);
		}

		// Note that there is activity
		activity();

		// Echo the message
		if(ws_.got_text()){
			this->callback::on_text(buffer_);
		}else{
			this->callback::on_binary(buffer_);
		}

		// Clear the buffer
		buffer_.consume(buffer_.size());

		// Do another read
		do_read();
	}

	template < typename Derived >
	void ws_session< Derived >::on_write(boost::system::error_code ec){
		// Happens when the timer closes the socket
		if(ec == boost::asio::error::operation_aborted){
			return;
		}

		if(ec){
			this->callback::on_error(error_type::write, ec);
			return;
		}
	}

	template < typename Derived >
	template < typename Data >
	void ws_session< Derived >::send(std::shared_ptr< Data > data){
		ws_.text(std::is_same< Data, std::string >::value);
		auto buffer = boost::asio::const_buffer(data->data(), data->size());
		ws_.async_write(
			std::move(buffer),
			boost::asio::bind_executor(
				strand_,
				[this_ = this->shared_from_this(), data = std::move(data)](
					boost::system::error_code ec,
					std::size_t /*bytes_transferred*/
				){
					this_->on_write(ec);
				}));
	}

	template < typename Derived >
	void ws_session< Derived >::send(
		boost::beast::websocket::close_reason reason
	){
		dispatch(strand_, [this, reason]{
			ws_.close(reason);
		});
	}

	template < typename Derived >
	void ws_session< Derived >::close(boost::system::error_code ec){
		// The timer expired while trying to handshake,
		// or we sent a ping and it never completed or
		// we never got back a control frame, so close.

		// Closing the socket cancels all outstanding operations.
		// They will complete with boost::asio::error::operation_aborted
		ws_.next_layer().shutdown(
			boost::asio::ip::tcp::socket::shutdown_both, ec);
		ws_.next_layer().close(ec);
	}


	template void ws_session< ws_server_session >
		::send< std::vector< std::uint8_t > >(
			std::shared_ptr< std::vector< std::uint8_t > > data
		);
	template void ws_session< ws_server_session >
		::send< std::string >(std::shared_ptr< std::string > data);

	template void ws_session< ws_server_session >
		::send(boost::beast::websocket::close_reason reason);


	ws_server_session::ws_server_session(
		ws_stream&& ws,
		ws_service_impl& service
	)
		: ws_session< ws_server_session >(std::move(ws))
		, service_(service) {}

	ws_server_session::~ws_server_session(){
		if(is_open_){
			this->callback::on_close();
		}
	}

	void ws_server_session::do_accept(
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
		start_timer();

		resource_ = std::string(req.target());

		// Accept the WebSocket handshake
		ws_.async_accept(
			req,
			boost::asio::bind_executor(
				strand_,
				[this_ = this->shared_from_this()]
				(boost::system::error_code ec){
					this_->on_accept(ec);
				}));
	}

	void ws_server_session::on_accept(boost::system::error_code ec){
		// Happens when the timer closes the socket
		if(ec == boost::asio::error::operation_aborted){
			return;
		}

		if(ec){
			this->callback::on_error(ws_service_error::accept, ec);
			return;
		}

		is_open_ = true;
		on_open();

		// Read a message
		do_read();
	}


	void ws_server_session::on_open()noexcept{
		try{
			service_.on_open(this, resource_);
		}catch(...){
			on_exception(std::current_exception());
		}
	}

	void ws_server_session::on_close(){
		service_.on_close(this, resource_);
	}

	void ws_server_session::on_text(
		boost::beast::multi_buffer& buffer
	){
		service_.on_text(this, resource_, buffer);
	}

	void ws_server_session::on_binary(
		boost::beast::multi_buffer& buffer
	){
		service_.on_binary(this, resource_, buffer);
	}

	void ws_server_session::on_error(
		ws_service_error error,
		boost::system::error_code ec
	){
		service_.on_error(this, resource_, error, ec);
	}

	void ws_server_session::on_exception(
		std::exception_ptr error
	)noexcept{
		service_.on_exception(this, resource_, error);
	}


	template void ws_session< ws_client_session >
		::send< std::vector< std::uint8_t > >(
			std::shared_ptr< std::vector< std::uint8_t > > data
		);
	template void ws_session< ws_client_session >
		::send< std::string >(std::shared_ptr< std::string > data);

	template void ws_session< ws_client_session >
		::send(boost::beast::websocket::close_reason reason);


	ws_client_session::ws_client_session(
		ws_stream&& ws,
		ws_client_impl& client
	)
		: ws_session< ws_client_session >(std::move(ws))
		, client_(client) {}

	ws_client_session::~ws_client_session(){
		this->callback::on_close();
	}


	void ws_client_session::start(){
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

		do_read();
	}

	void ws_client_session::on_close(){
		client_.on_close();
	}

	void ws_client_session::on_text(
		boost::beast::multi_buffer& buffer
	){
		client_.on_text(buffer);
	}

	void ws_client_session::on_binary(
		boost::beast::multi_buffer& buffer
	){
		client_.on_binary(buffer);
	}

	void ws_client_session::on_error(
		ws_client_error error,
		boost::system::error_code ec
	){
		client_.on_error(error, ec);
	}

	void ws_client_session::on_exception(
		std::exception_ptr error
	)noexcept{
		client_.on_exception(error);
	}



}
