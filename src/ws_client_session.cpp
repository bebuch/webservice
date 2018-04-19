//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <webservice/ws_client_session.hpp>
#include <webservice/ws_client_base.hpp>

#include "ws_session.ipp"


namespace webservice{


	template void ws_session< ws_client_session >
		::send(bool, shared_const_buffer);


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


	void ws_client_session::start()try{
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

		do_timer("ws_session::do_timer_start");
		restart_timer("ws_session::do_timer_restart_start");

		is_open_ = true;
		on_open();

		do_read("ws_client_session::start::read");
	}catch(...){
		close_socket();
		throw;
	}


	void ws_client_session::on_open()noexcept try{
		handler_strand_.defer(
			[this, lock = locker_.make_lock("ws_client_session::on_open")]{
				lock.enter();

				try{
					client_.on_open();
				}catch(...){
					on_exception(std::current_exception());
				}
			}, std::allocator< void >());
	}catch(...){
		on_exception(std::current_exception());
	}

	void ws_client_session::on_close()noexcept try{
		client_.on_close();
	}catch(...){
		on_exception(std::current_exception());
	}

	void ws_client_session::on_text(
		boost::beast::multi_buffer&& buffer
	)noexcept try{
		handler_strand_.defer(
			[
				this, lock = locker_.make_lock("ws_client_session::on_text"),
				buffer = std::move(buffer)
			]()mutable{
				lock.enter();

				try{
					client_.on_text(std::move(buffer));
				}catch(...){
					on_exception(std::current_exception());
				}
			}, std::allocator< void >());
	}catch(...){
		on_exception(std::current_exception());
	}

	void ws_client_session::on_binary(
		boost::beast::multi_buffer&& buffer
	)noexcept try{
		handler_strand_.defer(
			[
				this, lock = locker_.make_lock("ws_client_session::on_binary"),
				buffer = std::move(buffer)
			]()mutable{
				lock.enter();

				try{
					client_.on_binary(std::move(buffer));
				}catch(...){
					on_exception(std::current_exception());
				}
			}, std::allocator< void >());
	}catch(...){
		on_exception(std::current_exception());
	}

	void ws_client_session::on_error(
		boost::beast::string_view location,
		boost::system::error_code ec
	)noexcept{
		client_.on_exception(
			std::make_exception_ptr(boost::system::system_error(ec,
				"websocket client session " + std::string(location))));
	}

	void ws_client_session::on_exception(std::exception_ptr error)noexcept{
		client_.on_exception(error);
	}

	void ws_client_session::remove()noexcept{
		client_.remove_session();
	}


}
