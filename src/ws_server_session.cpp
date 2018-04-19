//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <webservice/ws_server_session.hpp>
#include <webservice/ws_service_interface.hpp>

#include "ws_session.ipp"


namespace webservice{


	template void ws_session< ws_server_session >
		::send(bool, shared_const_buffer);


	ws_server_session::ws_server_session(
		ws_stream&& ws,
		ws_service_interface& service,
		std::chrono::milliseconds ping_time
	)
		: ws_session< ws_server_session >(std::move(ws), ping_time)
		, service_(service) {}


	ws_server_session::~ws_server_session(){
		if(is_open_){
			on_close();
		}
	}


	void ws_server_session::do_accept(http_request&& req)try{
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

		do_timer("ws_session::do_timer_do_accept");
		restart_timer("ws_session::do_timer_restart_do_accept");

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
						on_error("accept", ec);
						return;
					}

					is_open_ = true;
					on_open();

					// Read a message
					do_read("ws_server_session::do_accept::read");
				}));
	}catch(...){
		close_socket();
		throw;
	}


	void ws_server_session::on_open()noexcept try{
		handler_strand_.defer(
			[this, lock = locker_.make_lock("ws_server_session::on_open")]{
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

	void ws_server_session::on_close()noexcept try{
		service_.on_close(ws_identifier(*this));
	}catch(...){
		on_exception(std::current_exception());
	}

	void ws_server_session::on_text(
		boost::beast::multi_buffer&& buffer
	)noexcept try{
		handler_strand_.defer(
			[
				this, lock = locker_.make_lock("ws_server_session::on_text"),
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

	void ws_server_session::on_binary(
		boost::beast::multi_buffer&& buffer
	)noexcept try{
		handler_strand_.defer(
			[
				this, lock = locker_.make_lock("ws_server_session::on_binary"),
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

	void ws_server_session::on_error(
		boost::beast::string_view location,
		boost::system::error_code ec
	)noexcept{
		on_exception(std::make_exception_ptr(boost::system::system_error(ec,
			"websocket server session " + std::string(location))));
	}

	void ws_server_session::on_exception(std::exception_ptr error)noexcept{
		service_.on_exception(ws_identifier(*this), error);
	}

	void ws_server_session::remove()noexcept{
		service_.on_erase(ws_identifier(*this));
	}


}
