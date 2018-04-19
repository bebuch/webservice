//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <webservice/ws_client_base.hpp>
#include <webservice/ws_client_session.hpp>

#include <boost/beast/core/multi_buffer.hpp>
#include <boost/beast/core/string.hpp>
#include <boost/beast/websocket.hpp>

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <memory>
#include <string>
#include <vector>
#include <thread>


namespace webservice{


	ws_client_base::ws_client_base()
		: strand_(ioc_.get_executor())
		, work_(boost::asio::make_work_guard(ioc_))
		, thread_([this]{
				// restart io_context if it returned by exception
				for(;;){
					try{
						ioc_.run();
						return;
					}catch(...){
						on_exception(std::current_exception());
					}
				}
			}) {}

	ws_client_base::~ws_client_base(){
		shutdown();
		block();
	}


	void ws_client_base::shutdown()noexcept{
		shutdown_ = true;
		strand_.dispatch(
			[this]{
				if(session_){
					session_->close("shutdown");
				}

				work_.reset();
			}, std::allocator< void >());
	}

	void ws_client_base::block()noexcept{
		std::lock_guard< std::mutex > lock(mutex_);
		if(thread_.joinable()){
			try{
				thread_.join();
			}catch(...){
				on_exception(std::current_exception());
			}
		}
	}

	void ws_client_base::async_connect(
		std::string host,
		std::string port,
		std::string resource
	){
		strand_.dispatch(
			[
				this,
				host = std::move(host),
				port = std::move(port),
				resource = [](std::string&& resource){
						if(resource.empty()) resource = "/";
						return std::move(resource);
					}(std::move(resource))
			]()mutable{
				if(is_connected()){
					throw std::logic_error("ws client is already connected");
				}

				if(shutdown_){
					remove_session();
					throw std::logic_error("can not connect after shutdown");
				}

				boost::asio::ip::tcp::resolver resolver(ioc_);
				auto results = resolver.resolve(host, port);

				ws_stream ws(ioc_);
				ws.read_message_max(max_read_message_size());

				// Make the connection on the IP address we get from a lookup
				boost::asio::connect(ws.next_layer(),
					results.begin(), results.end());

				// Perform the ws handshake
				ws.handshake(host, resource);

				// Create a WebSocket session by transferring the socket
				session_ = std::make_unique< ws_client_session >(
					std::move(ws), *this, ping_time());
				session_->start();
			}, std::allocator< void >());
	}

	bool ws_client_base::is_connected()const{
		return session_.get() != nullptr;
	}


	void ws_client_base::send_text(shared_const_buffer buffer){
		strand_.dispatch(
			[this, buffer = std::move(buffer)]()mutable{
				if(!session_){
					throw std::runtime_error(
						"ws send text: client is not connect");
				}

				session_->send(true, std::move(buffer));
			}, std::allocator< void >());
	}

	void ws_client_base::send_binary(shared_const_buffer buffer){
		strand_.dispatch(
			[this, buffer = std::move(buffer)]()mutable{
				if(!session_){
					throw std::runtime_error(
						"ws send binary: client is not connect");
				}

				session_->send(false, std::move(buffer));
			}, std::allocator< void >());
	}


	void ws_client_base::close(boost::beast::string_view reason){
		strand_.dispatch(
			[this, reason]{
				if(!session_){
					throw std::runtime_error(
						"ws send close: client is not connect");
				}

				session_->close(boost::beast::websocket::close_reason(reason));
			}, std::allocator< void >());
	}

	boost::asio::io_context::executor_type ws_client_base::get_executor(){
		return ioc_.get_executor();
	}

	void ws_client_base::do_poll_while(
		std::function< bool() > const& fn
	)noexcept{
		// As long as async calls are pending
		while(fn()){
			try{
				// Request the client to run a handler async
				if(ioc_.poll_one() == 0){
					// If no handler was waiting, the pending one must
					// currently run in another thread
					std::this_thread::yield();
				}
			}catch(...){
				on_exception(std::current_exception());
			}
		}
	}

	void ws_client_base::remove_session(){
		strand_.post([this]{
				session_.reset();
			}, std::allocator< void >());
	}


	void ws_client_base::on_open(){}

	void ws_client_base::on_close(){}

	void ws_client_base::on_text(
		boost::beast::multi_buffer&& /*buffer*/){}

	void ws_client_base::on_binary(
		boost::beast::multi_buffer&& /*buffer*/){}

	void ws_client_base::on_exception(std::exception_ptr /*error*/)noexcept{}


}
