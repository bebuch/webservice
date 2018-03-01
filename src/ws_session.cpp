//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include "ws_service_base_impl.hpp"
#include "ws_client_base_impl.hpp"

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
			std::chrono::steady_clock::time_point::max())
	{
		ws_.auto_fragment(true);
	}

	template < typename Derived >
	void ws_session< Derived >::on_timer(boost::system::error_code ec){
		if(std::is_same< Derived, ws_server_session >::value){
			std::cout << "server on_timer: " << ec.message() << "\n";
		}else{
			std::cout << "client on_timer: " << ec.message() << "\n";
		}

		if(ec && ec != boost::asio::error::operation_aborted){
			this->callback::on_error(location_type::timer, ec);
			return;
		}

		// See if the timer really expired since the deadline may have
		// moved.
		if(timer_.expiry() <= std::chrono::steady_clock::now()){
			if(std::is_same< Derived, ws_server_session >::value){
				std::cout << "server on_timer ping\n";
			}else{
				std::cout << "client on_timer ping\n";
			}

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
							if(std::is_same< Derived, ws_server_session >::value){
								std::cout << "server send ping: " << ec.message() << "\n";
							}else{
								std::cout << "client send ping: " << ec.message() << "\n";
							}
							this_->on_ping(ec);
						}));
			}else{
				if(std::is_same< Derived, ws_server_session >::value){
					std::cout << "server on_timer close\n";
				}else{
					std::cout << "client on_timer close\n";
				}

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
		timer_.expires_after(std::chrono::seconds(5));
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
			this->callback::on_error(location_type::ping, ec);
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
			this->callback::on_error(location_type::read, ec);
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
			this->callback::on_error(location_type::write, ec);
			return;
		}
	}

	template < typename Derived >
	template < typename Tag >
	void ws_session< Derived >::send(
		std::tuple< Tag, shared_const_buffer > data
	){
		ws_.text(std::is_same< Tag, text_tag >::value);
		ws_.async_write(
			std::get< 1 >(data),
			boost::asio::bind_executor(
				strand_,
				[this_ = this->shared_from_this()](
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
		if(std::is_same< Derived, ws_server_session >::value){
			std::cout << "server close: " << reason << "\n";
		}else{
			std::cout << "client close: " << reason << "\n";
		}

		ws_.async_close(
			reason,
			boost::asio::bind_executor(
				strand_,
				[this_ = this->shared_from_this()](
					boost::system::error_code ec
				){
					(void)ec; // TODO: handler error code
				}));
	}

	template < typename Derived >
	void ws_session< Derived >::close(boost::system::error_code ec){
		if(std::is_same< Derived, ws_server_session >::value){
			std::cout << "server close next_layer\n";
		}else{
			std::cout << "client close next_layer\n";
		}

		// The timer expired while trying to handshake,
		// or we sent a ping and it never completed or
		// we never got back a control frame, so close.

		// Closing the socket cancels all outstanding operations.
		// They will complete with boost::asio::error::operation_aborted
		ws_.next_layer().shutdown(
			boost::asio::ip::tcp::socket::shutdown_both, ec);
		ws_.next_layer().close(ec);
	}


	template void ws_session< ws_server_session >::send< text_tag >(
		std::tuple< text_tag, shared_const_buffer > data);

	template void ws_session< ws_server_session >::send< binary_tag >(
		std::tuple< binary_tag, shared_const_buffer > data);

	template void ws_session< ws_server_session >
		::send(boost::beast::websocket::close_reason reason);


	ws_server_session::ws_server_session(
		ws_stream&& ws,
		ws_service_base_impl& service
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
				boost::beast::websocket::frame_type kind,
				boost::beast::string_view payload
			){
				switch(kind){
					case boost::beast::websocket::frame_type::close:
						std::cout << "server close\n";
					break;
					case boost::beast::websocket::frame_type::ping: {
						std::cout << "server ping\n";
						ws_.async_pong(
							boost::beast::websocket::ping_data(payload),
							boost::asio::bind_executor(
								strand_,
								[this_ = this->shared_from_this()](
									boost::system::error_code ec
								){
									(void)ec; // TODO: handle error
								}));
					} break;
					case boost::beast::websocket::frame_type::pong:
						std::cout << "server pong\n";
					break;
				}
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
			this->callback::on_error(ws_service_location::accept, ec);
			return;
		}

		is_open_ = true;
		this->callback::on_open();

		// Read a message
		do_read();
	}


	void ws_server_session::on_open(){
		service_.on_open(this, resource_);
	}

	void ws_server_session::on_close(){
		service_.on_close(this, resource_);
	}

	void ws_server_session::on_text(
		boost::beast::multi_buffer const& buffer
	){
		service_.on_text(this, resource_, buffer);
	}

	void ws_server_session::on_binary(
		boost::beast::multi_buffer const& buffer
	){
		service_.on_binary(this, resource_, buffer);
	}

	void ws_server_session::on_error(
		ws_service_location location,
		boost::system::error_code ec
	){
		service_.on_error(this, resource_, location, ec);
	}

	void ws_server_session::on_exception(
		std::exception_ptr error
	)noexcept{
		service_.on_exception(this, resource_, error);
	}


	template void ws_session< ws_client_session >::send< text_tag >(
		std::tuple< text_tag, shared_const_buffer > data);

	template void ws_session< ws_client_session >::send< binary_tag >(
		std::tuple< binary_tag, shared_const_buffer > data);

	template void ws_session< ws_client_session >
		::send(boost::beast::websocket::close_reason reason);


	ws_client_session::ws_client_session(
		ws_stream&& ws,
		ws_client_base_impl& client
	)
		: ws_session< ws_client_session >(std::move(ws))
		, client_(client) {}

	ws_client_session::~ws_client_session(){
		if(is_open_){
			this->callback::on_close();
		}
	}


	void ws_client_session::start(){
		// Set the control callback. This will be called
		// on every incoming ping, pong, and close frame.
		ws_.control_callback(
			[this](
				boost::beast::websocket::frame_type kind,
				boost::beast::string_view payload
			){
				switch(kind){
					case boost::beast::websocket::frame_type::close:
						std::cout << "client close\n";
					break;
					case boost::beast::websocket::frame_type::ping: {
						std::cout << "client ping\n";
						ws_.async_pong(
							boost::beast::websocket::ping_data(payload),
							boost::asio::bind_executor(
								strand_,
								[this_ = this->shared_from_this()](
									boost::system::error_code ec
								){
									(void)ec; // TODO: handle error
								}));
					} break;
					case boost::beast::websocket::frame_type::pong:
						std::cout << "client pong\n";
					break;
				}
				// Note that there is activity
				activity();
			});


		// Run the timer. The timer is operated
		// continuously, this simplifies the code.
		on_timer({});

		boost::asio::asio_handler_invoke(
			boost::asio::bind_executor(
				strand_,
				[this_ = this->shared_from_this()]{
					this_->is_open_ = true;
					this_->callback::on_open();
				}));

		do_read();
	}


	void ws_client_session::on_open(){
		client_.on_open();
	}

	void ws_client_session::on_close(){
		client_.on_close();
	}

	void ws_client_session::on_text(
		boost::beast::multi_buffer const& buffer
	){
		client_.on_text(buffer);
	}

	void ws_client_session::on_binary(
		boost::beast::multi_buffer const& buffer
	){
		client_.on_binary(buffer);
	}

	void ws_client_session::on_error(
		ws_client_location location,
		boost::system::error_code ec
	){
		client_.on_error(location, ec);
	}

	void ws_client_session::on_exception(
		std::exception_ptr error
	)noexcept{
		client_.on_exception(error);
	}



}
