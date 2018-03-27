//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include "ws_session.hpp"

#include <webservice/ws_handler_base.hpp>
#include <webservice/ws_client_base.hpp>

#include <boost/beast/websocket.hpp>

#include <boost/asio/strand.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/dispatch.hpp>


namespace webservice{


	template < typename Derived >
	ws_session< Derived >::ws_session(
		ws_stream&& ws,
		boost::optional< std::chrono::milliseconds > websocket_ping_time
	)
		: ws_(std::move(ws))
		, strand_(ws_.get_executor())
		, write_strand_(ws_.get_executor())
		, write_list_(64)
		, websocket_ping_time_(websocket_ping_time)
		, timer_(ws_.get_executor().context(),
			std::chrono::steady_clock::time_point::max())
	{
		ws_.auto_fragment(true);
	}

	template < typename Derived >
	void ws_session< Derived >::do_timer(){
		timer_.async_wait(boost::asio::bind_executor(
			strand_,
			[this, lock = async_lock(async_calls_)]
			(boost::system::error_code ec){
				if(ec == boost::asio::error::operation_aborted){
					return;
				}

				if(ec){
					derived().on_error(location_type::timer, ec);
				}

				// If this is the first time the timer expired and ec was not
				// set, then send a ping to see if the other end is there.
				// Close the session otherwise.
				if(!ec && wait_on_pong_.exchange(true)){
					// Set the timer
					restart_timer();

					auto ping_payload =
						(std::is_same< Derived, ws_server_session >::value
								? "server " : "client ")
							+ std::to_string(ping_counter_++);

					// Now send the ping
					ws_.async_ping(
						boost::beast::websocket::ping_data(
							ping_payload.c_str(), ping_payload.size()),
						boost::asio::bind_executor(
							strand_,
							[this, lock = async_lock(async_calls_)](
								boost::system::error_code ec
							){
								// Happens when the timer closes the socket
								if(ec == boost::asio::error::operation_aborted){
									return;
								}

								if(ec){
									derived().on_error(location_type::ping, ec);
									return;
								}
							}));

					do_timer();
				}else{
					// Closing the socket cancels all outstanding operations.
					// They will complete with operation_aborted
					using socket = boost::asio::ip::tcp::socket;
					ws_.next_layer().shutdown(socket::shutdown_both, ec);
					ws_.next_layer().close(ec);
					async_erase();
				}
			}));
	}

	template < typename Derived >
	void ws_session< Derived >::restart_timer(){
		if(websocket_ping_time_){
			timer_.expires_after(*websocket_ping_time_);
		}
	}

	template < typename Derived >
	void ws_session< Derived >::activity() {
		// Note that the session is alive
		wait_on_pong_ = false;

		restart_timer();
	}

	template < typename Derived >
	void ws_session< Derived >::do_read(){
		restart_timer();

		// Read a message into our buffer
		if(ws_.is_open()){
			ws_.async_read(
				buffer_,
				boost::asio::bind_executor(
					strand_,
					[this, lock = async_lock(async_calls_)](
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

		// Note that there is activity
		activity();

		if(ec){
			derived().on_error(location_type::read, ec);
		}else{
			// Echo the message
			if(ws_.got_text()){
				derived().on_text(buffer_);
			}else{
				derived().on_binary(buffer_);
			}
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
			derived().on_error(location_type::write, ec);
		}

		write_list_.pop_front();
		if(!write_list_.empty()){
			do_write();
		}
	}

	template < typename Derived >
	template < typename Tag >
	void ws_session< Derived >::send(
		std::tuple< Tag, shared_const_buffer > data
	){
		boost::asio::asio_handler_invoke(boost::asio::bind_executor(
			write_strand_,
			[
				this, lock = async_lock(async_calls_),
				data = std::move(std::get< 1 >(data))
			]()mutable{
				if(this_->write_list_.full()){
					throw std::runtime_error("write buffer is full");
				}

				bool was_empty = this_->write_list_.empty();

				constexpr bool is_text = std::is_same< Tag, text_tag >::value;
				this_->write_list_.push_back(
					write_data{is_text, std::move(data)});

				if(was_empty){
					this_->do_write();
				}
			}));
	}

	template < typename Derived >
	void ws_session< Derived >::do_write(){
		ws_.text(write_list_.front().is_text);
		ws_.async_write(
			std::move(write_list_.front().data),
			boost::asio::bind_executor(
				write_strand_,
				[this, lock = async_lock(async_calls_)](
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
		ws_.async_close(
			reason,
			boost::asio::bind_executor(
				strand_,
				[this, lock = async_lock(async_calls_)](
					boost::system::error_code ec
				){
					if(ec){
						this_->on_error(location_type::close, ec);
					}
				}));
	}

	template < typename Derived >
	void ws_session< Derived >::async_erase(){
		std::call_once(erase_flag_, [this]{
				boost::asio::post([this]{
						eraser_();
					});
			});
	}


	template void ws_session< ws_server_session >::send< text_tag >(
		std::tuple< text_tag, shared_const_buffer > data);

	template void ws_session< ws_server_session >::send< binary_tag >(
		std::tuple< binary_tag, shared_const_buffer > data);

	template void ws_session< ws_server_session >
		::send(boost::beast::websocket::close_reason reason);


	ws_server_session::ws_server_session(
		ws_stream&& ws,
		ws_handler_base& service,
		boost::optional< std::chrono::milliseconds > websocket_ping_time
	)
		: ws_session< ws_server_session >(std::move(ws), websocket_ping_time)
		, service_(&service) {}

	ws_server_session::~ws_server_session(){
		if(ws_.is_open()){
			try{
				ws_.close("server shutdown");
			}catch(...){
				on_exception(std::current_exception());
			}
		}

		if(is_open_){
			on_close();
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
		restart_timer();

		resource_ = std::string(req.target());

		// Accept the WebSocket handshake
		ws_.async_accept(
			req,
			boost::asio::bind_executor(
				strand_,
				[this, lock = async_lock(async_calls_)]
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
			on_error(ws_handler_location::accept, ec);
			return;
		}

		is_open_ = true;
		on_open();

		// Read a message
		do_read();
	}


	void ws_server_session::on_open()noexcept{
		ws_handler_base* service = service_;
		if(service){
			try{
				service->on_open(this, resource_);
			}catch(...){
				on_exception(std::current_exception());
			}
		}
	}

	void ws_server_session::on_close()noexcept{
		ws_handler_base* service = service_;
		if(service){
			try{
				service->on_close(this, resource_);
			}catch(...){
				on_exception(std::current_exception());
			}
		}
	}

	void ws_server_session::on_text(
		boost::beast::multi_buffer const& buffer
	)noexcept{
		ws_handler_base* service = service_;
		if(service){
			try{
				service->on_text(this, resource_, buffer);
			}catch(...){
				on_exception(std::current_exception());
			}
		}
	}

	void ws_server_session::on_binary(
		boost::beast::multi_buffer const& buffer
	)noexcept{
		ws_handler_base* service = service_;
		if(service){
			try{
				service->on_binary(this, resource_, buffer);
			}catch(...){
				on_exception(std::current_exception());
			}
		}
	}

	void ws_server_session::on_error(
		ws_handler_location location,
		boost::system::error_code ec
	)noexcept{
		ws_handler_base* service = service_;
		if(service){
			try{
				service->on_error(this, resource_, location, ec);
			}catch(...){
				on_exception(std::current_exception());
			}
		}
	}

	void ws_server_session::on_exception(
		std::exception_ptr error
	)noexcept{
		ws_handler_base* service = service_;
		if(service){
			service->on_exception(this, resource_, error);
		}
	}


	void ws_server_session::rebind(ws_handler_base* service)noexcept{
		service_ = service;
	}


	template void ws_session< ws_client_session >::send< text_tag >(
		std::tuple< text_tag, shared_const_buffer > data);

	template void ws_session< ws_client_session >::send< binary_tag >(
		std::tuple< binary_tag, shared_const_buffer > data);

	template void ws_session< ws_client_session >
		::send(boost::beast::websocket::close_reason reason);


	ws_client_session::ws_client_session(
		ws_stream&& ws,
		ws_client_base& client,
		boost::optional< std::chrono::milliseconds > websocket_ping_time
	)
		: ws_session< ws_client_session >(std::move(ws), websocket_ping_time)
		, client_(client) {}

	ws_client_session::~ws_client_session(){
		if(ws_.is_open()){
			try{
				ws_.close("client shutdown");
			}catch(...){
				on_exception(std::current_exception());
			}
		}

		if(is_open_){
			on_close();
		}
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

		boost::asio::asio_handler_invoke(
			boost::asio::bind_executor(
				strand_,
				[this, lock = async_lock(async_calls_)]{
					this_->is_open_ = true;
					this_->on_open();
				}));

		do_read();
	}


	void ws_client_session::on_open()noexcept{
		try{
			client_.on_open();
		}catch(...){
			on_exception(std::current_exception());
		}
	}

	void ws_client_session::on_close()noexcept{
		try{
			client_.on_close();
		}catch(...){
			on_exception(std::current_exception());
		}
	}

	void ws_client_session::on_text(
		boost::beast::multi_buffer const& buffer
	)noexcept{
		try{
			client_.on_text(buffer);
		}catch(...){
			on_exception(std::current_exception());
		}
	}

	void ws_client_session::on_binary(
		boost::beast::multi_buffer const& buffer
	)noexcept{
		try{
			client_.on_binary(buffer);
		}catch(...){
			on_exception(std::current_exception());
		}
	}

	void ws_client_session::on_error(
		ws_client_location location,
		boost::system::error_code ec
	)noexcept{
		try{
			client_.on_error(location, ec);
		}catch(...){
			on_exception(std::current_exception());
		}
	}

	void ws_client_session::on_exception(
		std::exception_ptr error
	)noexcept{
		client_.on_exception(error);
	}



}
