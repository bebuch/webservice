//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include "ws_session.hpp"
#include "server_impl.hpp"

#include <webservice/server.hpp>
#include <webservice/async_lock.hpp>
#include <webservice/ws_handler_base.hpp>
#include <webservice/ws_client_base.hpp>

#include <boost/beast/websocket.hpp>

#include <boost/asio/strand.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/bind_executor.hpp>


namespace webservice{


	std::mutex async_locker::lock::mutex;

	std::atomic< std::size_t > async_locker::lock::counter{0};


	template < typename Derived >
	ws_session< Derived >::ws_session(
		ws_stream&& ws,
		std::chrono::milliseconds ping_time
	)
		: ws_(std::move(ws))
		, strand_(ws_.get_executor())
		, handler_strand_(ws_.get_executor())
		, timer_(ws_.get_executor().context(),
			std::chrono::steady_clock::time_point::max())
		, locker_([this]()noexcept{
				derived().remove();
			})
		, write_list_(64)
		, ping_time_(ping_time)
	{
		ws_.auto_fragment(true);
	}

	template < typename Derived >
	void ws_session< Derived >::do_timer(){
		timer_.async_wait(boost::asio::bind_executor(
			strand_,
			[this, lock = locker_.make_lock("ws_session::do_timer")]
			(boost::system::error_code ec){
				lock.enter();

				if(ec == boost::asio::error::operation_aborted){
					return;
				}

				if(ec){
					derived().on_error(location_type::timer, ec);
				}

				// If this is the first time the timer expired and ec was not
				// set, then send a ping to see if the other end is there.
				// Close the session otherwise.
				if(!ec && !wait_on_pong_){
					wait_on_pong_ = true;

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
							[this, lock = locker_.make_lock("ws_session::do_timer_intern")](
								boost::system::error_code ec
							){
								lock.enter();

								// Happens when the timer closes the socket
								if(ec == boost::asio::error::operation_aborted){
									return;
								}

								if(ec){
									derived().on_error(location_type::ping, ec);
								}
							}));

					do_timer();
				}else{
					// Closing the socket cancels all outstanding operations.
					// They will complete with operation_aborted
					using socket = boost::asio::ip::tcp::socket;
					ws_.next_layer().shutdown(socket::shutdown_both, ec);
					ws_.next_layer().close(ec);
				}
			}));
	}

	template < typename Derived >
	void ws_session< Derived >::restart_timer(){
		timer_.expires_after(ping_time_);
	}

	template < typename Derived >
	void ws_session< Derived >::activity() {
		// Note that the session is alive
		wait_on_pong_ = false;

		restart_timer();
	}

	template < typename Derived >
	void ws_session< Derived >::do_read(char const* start_pos){
		if(!ws_.is_open()){
			return;
		}

		restart_timer();

		// Read a message into our buffer
		ws_.async_read(
			buffer_,
			boost::asio::bind_executor(
				strand_,
				[this, lock = locker_.make_lock(start_pos)](
					boost::system::error_code ec,
					std::size_t /*bytes_transferred*/
				){
					lock.enter();

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
							derived().on_text(std::move(buffer_));
						}else{
							derived().on_binary(std::move(buffer_));
						}
					}

					// Clear the buffer
					buffer_.consume(buffer_.size());

					// Do another read
					do_read("ws_session::do_read");
				}));
	}


	template < typename Derived >
	template < typename Tag >
	void ws_session< Derived >::send(
		std::tuple< Tag, shared_const_buffer > data
	){
		strand_.dispatch(
			[
				this, lock = locker_.make_lock("ws_session::send"),
				data = std::move(std::get< 1 >(data))
			]()mutable{
				lock.enter();

				if(write_list_.full()){
					throw std::runtime_error("write buffer is full");
				}

				bool was_empty = write_list_.empty();

				constexpr bool is_text = std::is_same< Tag, text_tag >::value;
				write_list_.push_back(write_data{is_text, std::move(data)});

				if(was_empty){
					do_write();
				}
			}, std::allocator< void >());
	}

	template < typename Derived >
	void ws_session< Derived >::send(
		boost::beast::websocket::close_reason reason
	){
		strand_.post(
			[this, lock = locker_.make_lock("ws_session::send close"), reason]{
				lock.enter();

				if(ws_.is_open()){
					boost::system::error_code ec;
					ws_.close(reason, ec);
					if(ec){
						derived().on_error(location_type::close, ec);
					}
				}
			}, std::allocator< void >());
	}

	template < typename Derived >
	void ws_session< Derived >::do_write(){
		ws_.text(write_list_.front().is_text);
		ws_.async_write(
			std::move(write_list_.front().data),
			boost::asio::bind_executor(
				strand_,
				[this, lock = locker_.make_lock("ws_session::do_write")](
					boost::system::error_code ec,
					std::size_t /*bytes_transferred*/
				){
					lock.enter();

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
				}));
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
		std::chrono::milliseconds ping_time
	)
		: ws_session< ws_server_session >(std::move(ws), ping_time)
		, service_(service) {}


	ws_server_session::~ws_server_session(){
		if(is_open_){
			on_close();
		}
	}


	void ws_server_session::do_accept(http_request&& req){
		// lock until the first async operations has been started
		auto lock = locker_.make_first_lock("ws_server_session::do_accept::first");

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

		do_timer();
		restart_timer();

		resource_ = std::string(req.target());

		// Accept the WebSocket handshake
		ws_.async_accept(
			std::move(req),
			boost::asio::bind_executor(
				strand_,
				[this, lock = locker_.make_lock("ws_server_session::do_accept")]
				(boost::system::error_code ec){
					lock.enter();

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
					do_read("ws_server_session::do_accept::read");
				}));
	}


	void ws_server_session::on_open()noexcept{
		handler_strand_.defer(
			[this, lock = locker_.make_lock("ws_server_session::on_open")]{
				lock.enter();

				try{
					service_.on_open(ws_identifier(this), resource_);
				}catch(...){
					service_.on_exception(ws_identifier(this), resource_,
						std::current_exception());
				}
			}, std::allocator< void >());
	}

	void ws_server_session::on_close()noexcept{
		try{
			service_.on_close(ws_identifier(this), resource_);
		}catch(...){
			service_.on_exception(ws_identifier(this), resource_,
				std::current_exception());
		}
	}

	void ws_server_session::on_text(
		boost::beast::multi_buffer&& buffer
	)noexcept{
		handler_strand_.defer(
			[
				this, lock = locker_.make_lock("ws_server_session::on_text"),
				buffer = std::move(buffer)
			]()mutable{
				lock.enter();

				try{
					service_.on_text(
						ws_identifier(this), resource_, std::move(buffer));
				}catch(...){
					service_.on_exception(ws_identifier(this), resource_,
						std::current_exception());
				}
			}, std::allocator< void >());
	}

	void ws_server_session::on_binary(
		boost::beast::multi_buffer&& buffer
	)noexcept{
		handler_strand_.defer(
			[
				this, lock = locker_.make_lock("ws_server_session::on_binary"),
				buffer = std::move(buffer)
			]()mutable{
				lock.enter();

				try{
					service_.on_binary(
						ws_identifier(this), resource_, std::move(buffer));
				}catch(...){
					service_.on_exception(ws_identifier(this), resource_,
						std::current_exception());
				}
			}, std::allocator< void >());
	}

	void ws_server_session::on_error(
		ws_handler_location location,
		boost::system::error_code ec
	)noexcept{
		handler_strand_.defer(
			[this, lock = locker_.make_lock("ws_server_session::on_error"), location, ec]{
				lock.enter();

				try{
					service_.on_error(
						ws_identifier(this), resource_, location, ec);
				}catch(...){
					service_.on_exception(ws_identifier(this), resource_,
						std::current_exception());
				}
			}, std::allocator< void >());
	}

	void ws_server_session::on_exception(
		std::exception_ptr error
	)noexcept{
		handler_strand_.defer(
			[this, lock = locker_.make_lock("ws_server_session::on_exception"), error]{
				lock.enter();

				service_.on_exception(ws_identifier(this), resource_, error);
			}, std::allocator< void >());
	}

	void ws_server_session::remove()noexcept{
		service_.async_erase(this);
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
		std::chrono::milliseconds ping_time
	)
		: ws_session< ws_client_session >(std::move(ws), ping_time)
		, client_(client) {}


	ws_client_session::~ws_client_session(){
		if(is_open_){
			on_close();
		}
	}


	void ws_client_session::start(){
		// lock until the first async operations has been started
		auto lock = locker_.make_first_lock("ws_client_session::start::first");

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

		do_timer();

		is_open_ = true;
		on_open();

		do_read("ws_client_session::start::read");
	}


	void ws_client_session::on_open()noexcept{
		handler_strand_.defer(
			[this, lock = locker_.make_lock("ws_client_session::on_open")]{
				lock.enter();

				try{
					client_.on_open();
				}catch(...){
					client_.on_exception(std::current_exception());
				}
			}, std::allocator< void >());
	}

	void ws_client_session::on_close()noexcept{
		try{
			client_.on_close();
		}catch(...){
			client_.on_exception(std::current_exception());
		}
	}

	void ws_client_session::on_text(
		boost::beast::multi_buffer&& buffer
	)noexcept{
		handler_strand_.defer(
			[
				this, lock = locker_.make_lock("ws_client_session::on_text"),
				buffer = std::move(buffer)
			]()mutable{
				lock.enter();

				try{
					client_.on_text(std::move(buffer));
				}catch(...){
					client_.on_exception(std::current_exception());
				}
			}, std::allocator< void >());
	}

	void ws_client_session::on_binary(
		boost::beast::multi_buffer&& buffer
	)noexcept{
		handler_strand_.defer(
			[
				this, lock = locker_.make_lock("ws_client_session::on_binary"),
				buffer = std::move(buffer)
			]()mutable{
				lock.enter();

				try{
					client_.on_binary(std::move(buffer));
				}catch(...){
					client_.on_exception(std::current_exception());
				}
			}, std::allocator< void >());
	}

	void ws_client_session::on_error(
		ws_client_location location,
		boost::system::error_code ec
	)noexcept{
		handler_strand_.defer(
			[this, lock = locker_.make_lock("ws_client_session::on_error"), location, ec]{
				lock.enter();

				try{
					client_.on_error(location, ec);
				}catch(...){
					client_.on_exception(std::current_exception());
				}
			}, std::allocator< void >());
	}

	void ws_client_session::on_exception(
		std::exception_ptr error
	)noexcept{
		handler_strand_.defer(
			[this, lock = locker_.make_lock("ws_client_session::on_exception"), error]{
				lock.enter();

				client_.on_exception(error);
			}, std::allocator< void >());
	}

	void ws_client_session::remove()noexcept{
		client_.remove_session();
	}


}
