//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <webservice/async_lock.hpp>
#include <webservice/ws_session.hpp>
#include <webservice/ws_service_interface.hpp>

#include <boost/beast/websocket.hpp>

#include <boost/asio/strand.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/bind_executor.hpp>


namespace webservice{


	ws_session::ws_session(
		ws_stream&& ws,
		ws_service_interface& service,
		std::chrono::milliseconds ping_time
	)
		: service_(service)
		, ws_(std::move(ws))
		, strand_(ws_.get_executor())
		, handler_strand_(ws_.get_executor())
		, timer_(ws_.get_executor().context(),
			std::chrono::steady_clock::time_point::max())
		, locker_([this]()noexcept{
				service_.on_erase(ws_identifier(*this));
			})
		, write_list_(64)
		, ping_time_(ping_time)
	{
		ws_.auto_fragment(true);
		ws_.control_callback(
			[this](
				boost::beast::websocket::frame_type /*kind*/,
				boost::beast::string_view /*payload*/
			){
				// Note that there is activity
				activity();
			});
	}

	ws_session::~ws_session(){
		if(is_open_){
			on_close();
		}
	}


	void ws_session::do_accept(http_request&& req)try{
		// lock until the first async operations has been started
		auto lock = locker_.make_first_lock("ws_session::do_accept::first");

		start_timer();

		// Accept the WebSocket handshake
		ws_.async_accept(
			std::move(req),
			boost::asio::bind_executor(
				strand_,
				[this, lock = locker_.make_lock("ws_session::do_accept")]
				(boost::system::error_code ec){
					lock.enter();

					// Happens when the timer closes the socket
					if(ec == boost::asio::error::operation_aborted){
						return;
					}

					if(ec){
						on_error("accept", ec);
						return;
					}

					is_open_ = true;
					on_open();

					// Read a message
					do_read("ws_session::do_accept::read");
				}));
	}catch(...){
		close_socket();
		throw;
	}


	void ws_session::start()try{
		// lock until the first async operations has been started
		auto lock = locker_.make_first_lock("ws_session::start::first");

		start_timer();

		is_open_ = true;
		on_open();

		do_read("ws_session::start::read");
	}catch(...){
		close_socket();
		throw;
	}


	void ws_session::send(
		bool is_text,
		shared_const_buffer data
	)noexcept try{
		strand_.dispatch(
			[
				this, lock = locker_.make_lock("ws_session::send"),
				is_text,
				data = std::move(data)
			]()mutable{
				lock.enter();

				if(!ws_.is_open()){
					timer_.cancel();
					return;
				}

				if(close_reason_){
					return;
				}

				if(write_list_.full()){
					throw std::runtime_error("write buffer is full");
				}

				bool was_empty = write_list_.empty();

				write_list_.push_back(write_data{is_text, std::move(data)});

				if(was_empty){
					do_write();
				}
			}, std::allocator< void >());
	}catch(...){
		on_exception(std::current_exception());
	}

	void ws_session::close(
		boost::beast::websocket::close_reason reason
	)noexcept try{
		strand_.dispatch(
			[
				this, lock = locker_.make_lock("ws_session::send"),
				reason
			]{
				lock.enter();

				if(!ws_.is_open()){
					timer_.cancel();
					return;
				}

				if(close_reason_){
					return;
				}

				using close_reason = boost::beast::websocket::close_reason;
				close_reason_ = std::make_unique< close_reason >(reason);

				if(write_list_.empty()){
					do_write();
				}
			}, std::allocator< void >());
	}catch(...){
		on_exception(std::current_exception());
	}


	void ws_session::start_timer(){
		do_timer("ws_session::do_timer_start");
		restart_timer("ws_session::do_timer_restart_start");
	}

	void ws_session::do_timer(char const* op){
		timer_.async_wait(boost::asio::bind_executor(
			strand_,
			[this, lock = locker_.make_lock(op)]
			(boost::system::error_code ec){
				lock.enter();

				if(ec == boost::asio::error::operation_aborted){
					return;
				}

				// If this is the first time the timer expired and ec was not
				// set, then send a ping to see if the other end is there.
				// Close the session otherwise.
				if(!ec && !wait_on_pong_){
					if(!ws_.is_open()){
						return;
					}

					wait_on_pong_ = true;

					// Set the timer
					restart_timer("ws_session::restart_timer_recursion");

					auto ping_payload = std::to_string(ping_counter_++);

					// Now send the ping
					ws_.async_ping(
						boost::beast::websocket::ping_data(
							ping_payload.c_str(), ping_payload.size()),
						boost::asio::bind_executor(
							strand_,
							[this, lock = locker_.make_lock("ws_session::do_timer_ping")](
								boost::system::error_code ec
							){
								lock.enter();

								// Happens when the timer closes the socket
								if(ec == boost::asio::error::operation_aborted){
									return;
								}

								if(ec){
									on_error("ping", ec);
									close("ping error");
									return;
								}
							}));

					// timer was not cancel in restart_timer, so we have to
					// manuelly restart it
					do_timer("ws_session::do_timer_recursion");
				}else{
					if(ec){
						on_error("timer", ec);
					}

					close_socket();
				}
			}));
	}

	void ws_session::close_socket()noexcept{
		// Closing the socket cancels all outstanding operations.
		// They will complete with operation_aborted
		using socket = boost::asio::ip::tcp::socket;
		boost::system::error_code ec;
		ws_.next_layer().shutdown(socket::shutdown_both, ec);
		ws_.next_layer().close(ec);

		stop_timer();
	}


	void ws_session::stop_timer()noexcept{
		try{
			timer_.cancel();
		}catch(...){
			on_exception(std::current_exception());
		}
	}

	void ws_session::restart_timer(char const* op){
		if(timer_.expires_after(ping_time_) != 0 && ws_.is_open()){
			do_timer(op);
		}
	}

	void ws_session::activity(){
		// Note that the session is alive
		wait_on_pong_ = false;

		restart_timer("ws_session::do_timer_restart_activity");
	}

	void ws_session::do_read(char const* start_pos){
		if(!ws_.is_open()){
			return;
		}

		restart_timer("ws_session::do_timer_restart_do_read");

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
						timer_.cancel();
						return;
					}

					// Note that there is activity
					activity();

					if(ec){
						on_error("read", ec);

						if(ws_.is_open()){
							// close connection
							close("read error");

							// Do another read
							do_read("ws_session::do_read_after_close");
						}

						return;
					}

					// Echo the message
					if(ws_.got_text()){
						on_text(std::move(buffer_));
					}else{
						on_binary(std::move(buffer_));
					}

					// Do another read
					do_read("ws_session::do_read");
				}));
	}


	void ws_session::do_write(){
		if(close_reason_){
			ws_.async_close(*close_reason_, boost::asio::bind_executor(
				strand_,
				[this, lock = locker_.make_lock("ws_session::send_close")](
					boost::system::error_code ec
				){
					lock.enter();

					if(ec){
						on_error("close", ec);
					}

					if(!ws_.is_open()){
						stop_timer();
					}
				}));
		}else{
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
							on_error("write", ec);
							close("write error");
							return;
						}

						write_list_.pop_front();

						if(
							ws_.is_open() &&
							(!write_list_.empty() || close_reason_)
						){
							do_write();
						}
					}));
		}
	}


	void ws_session::on_open()noexcept try{
		handler_strand_.defer(
			[this, lock = locker_.make_lock("ws_session::on_open")]{
				lock.enter();

				try{
					service_.on_open(ws_identifier(*this));
				}catch(...){
					on_exception(std::current_exception());
				}
			}, std::allocator< void >());
	}catch(...){
		on_exception(std::current_exception());
	}

	void ws_session::on_close()noexcept try{
		service_.on_close(ws_identifier(*this));
	}catch(...){
		on_exception(std::current_exception());
	}

	void ws_session::on_text(
		boost::beast::multi_buffer&& buffer
	)noexcept try{
		handler_strand_.defer(
			[
				this, lock = locker_.make_lock("ws_session::on_text"),
				buffer = std::move(buffer)
			]()mutable{
				lock.enter();

				try{
					service_.on_text(ws_identifier(*this), std::move(buffer));
				}catch(...){
					on_exception(std::current_exception());
				}
			}, std::allocator< void >());
	}catch(...){
		on_exception(std::current_exception());
	}

	void ws_session::on_binary(
		boost::beast::multi_buffer&& buffer
	)noexcept try{
		handler_strand_.defer(
			[
				this, lock = locker_.make_lock("ws_session::on_binary"),
				buffer = std::move(buffer)
			]()mutable{
				lock.enter();

				try{
					service_.on_binary(ws_identifier(*this), std::move(buffer));
				}catch(...){
					on_exception(std::current_exception());
				}
			}, std::allocator< void >());
	}catch(...){
		on_exception(std::current_exception());
	}

	void ws_session::on_error(
		boost::beast::string_view location,
		boost::system::error_code ec
	)noexcept{
		on_exception(std::make_exception_ptr(boost::system::system_error(ec,
			"websocket session " + std::string(location))));
	}

	void ws_session::on_exception(std::exception_ptr error)noexcept{
		service_.on_exception(ws_identifier(*this), error);
	}


}
