//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <webservice/ws_client_base.hpp>

#include "ws_session.hpp"

#include <boost/beast/core/multi_buffer.hpp>
#include <boost/beast/core/string.hpp>
#include <boost/beast/websocket.hpp>

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <mutex>


namespace webservice{


	struct ws_client_base_impl{
		/// \brief Constructor
		ws_client_base_impl(
			std::string&& host,
			std::string&& port,
			std::string&& resource
		)
			: host_(std::move(host))
			, port_(std::move(port))
			, resource_([](std::string&& resource){
					if(resource.empty()) resource = "/";
					return std::move(resource);
				}(std::move(resource)))
			, work_(boost::asio::make_work_guard(ioc_)) {}


		/// \brief Send a message
		template < typename Data >
		void send(Data&& data){
			std::lock_guard< std::recursive_mutex > lock(mutex_);
			if(session_){
				session_->send(static_cast< Data&& >(data));
			}
		}

		std::string const host_;
		std::string const port_;
		std::string const resource_;

		std::atomic< bool > shutdown_{false};
		std::recursive_mutex mutex_;

		boost::asio::io_context ioc_;
		boost::asio::executor_work_guard<
			boost::asio::io_context::executor_type > work_;
		std::unique_ptr< ws_client_session > session_;
		std::thread thread_;
	};


	ws_client_base::ws_client_base(
		std::string host,
		std::string port,
		std::string resource
	)
		: impl_(std::make_unique< ws_client_base_impl >(
			std::move(host), std::move(port), std::move(resource))) {}

	ws_client_base::~ws_client_base(){
		shutdown();
		block();
	}


	void ws_client_base::connect(){
		std::lock_guard< std::recursive_mutex > lock(impl_->mutex_);
		if(is_connected()){
			return;
		}

		block();

		boost::asio::ip::tcp::resolver resolver(impl_->ioc_);
		auto results = resolver.resolve(impl_->host_, impl_->port_);

		ws_stream ws(impl_->ioc_);
		ws.read_message_max(max_read_message_size());

		// Make the connection on the IP address we get from a lookup
		boost::asio::connect(ws.next_layer(),
			results.begin(), results.end());

		// Perform the ws handshake
		ws.handshake(impl_->host_, impl_->resource_);

		// Create a WebSocket session by transferring the socket
		impl_->session_ = std::make_unique< ws_client_session >(
			std::move(ws), *this, ping_time());
		impl_->session_->set_erase_fn(&impl_->session_);
		impl_->session_->start();


		// restart io_context if it returned by exception
		impl_->thread_ = std::thread([this]{
				for(;;){
					try{
						impl_->ioc_.run();
						return;
					}catch(...){
						on_exception(std::current_exception());
					}
				}
			});
	}

	bool ws_client_base::is_connected()const{
		return static_cast< bool >(impl_->session_.get());
	}


	void ws_client_base::send_text(shared_const_buffer buffer){
		impl_->send(std::make_tuple(text_tag{}, std::move(buffer)));
	}

	void ws_client_base::send_binary(shared_const_buffer buffer){
		impl_->send(std::make_tuple(binary_tag{}, std::move(buffer)));
	}


	void ws_client_base::close(boost::beast::string_view reason){
		impl_->send(boost::beast::websocket::close_reason(reason));
	}

	void ws_client_base::block()noexcept{
		std::lock_guard< std::recursive_mutex > lock(impl_->mutex_);
		if(impl_->thread_.joinable()){
			try{
				impl_->thread_.join();
			}catch(...){
				on_exception(std::current_exception());
			}
		}
	}

	void ws_client_base::shutdown()noexcept{
		if(!impl_->shutdown_.exchange(true)){
			if(impl_->session_){
				impl_->session_->async_erase();
			}
			impl_->work_.reset();
		}
	}

	boost::asio::io_context::executor_type ws_client_base::get_executor(){
		return impl_->ioc_.get_executor();
	}

	std::size_t ws_client_base::poll_one()noexcept{
		try{
			return impl_->ioc_.poll_one();
		}catch(...){
			on_exception(std::current_exception());
			return 1;
		}
	}


	void ws_client_base::on_open(){}

	void ws_client_base::on_close(){}

	void ws_client_base::on_text(
		boost::beast::multi_buffer&& /*buffer*/){}

	void ws_client_base::on_binary(
		boost::beast::multi_buffer&& /*buffer*/){}

	void ws_client_base::on_error(
		ws_client_location /*location*/,
		boost::system::error_code /*ec*/){}

	void ws_client_base::on_exception(std::exception_ptr /*error*/)noexcept{}


}
