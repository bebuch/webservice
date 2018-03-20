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

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/defer.hpp>

#include <boost/make_unique.hpp>

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
			std::string&& resource,
			boost::optional< std::chrono::milliseconds > websocket_ping_time,
			std::size_t max_read_message_size
		)
			: host_(std::move(host))
			, port_(std::move(port))
			, resource_([](std::string&& resource){
					if(resource.empty()) resource = "/";
					return std::move(resource);
				}(std::move(resource)))
			, websocket_ping_time_(websocket_ping_time)
			, max_read_message_size_(max_read_message_size) {}


		/// \brief Send a message
		template < typename Data >
		void send(Data&& data){
			if(auto const session = session_.lock()){
				session->send(static_cast< Data&& >(data));
			}
		}

		std::string const host_;
		std::string const port_;
		std::string const resource_;

		std::recursive_mutex mutex_;

		boost::asio::io_context ioc_;
		boost::optional< std::chrono::milliseconds > const websocket_ping_time_;
		std::size_t const max_read_message_size_;
		std::weak_ptr< ws_client_session > session_;
		std::thread thread_;
	};


	ws_client_base::ws_client_base(
		std::string host,
		std::string port,
		std::string resource,
		boost::optional< std::chrono::milliseconds > websocket_ping_time,
		std::size_t max_read_message_size
	)
		: impl_(boost::make_unique< ws_client_base_impl >(
			std::move(host), std::move(port), std::move(resource),
			websocket_ping_time, max_read_message_size)) {}

	ws_client_base::~ws_client_base(){
		impl_->send("client shutdown");
		stop();
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
		ws.read_message_max(impl_->max_read_message_size_);

		// Make the connection on the IP address we get from a lookup
		boost::asio::connect(ws.next_layer(),
			results.begin(), results.end());

		// Perform the ws handshake
		ws.handshake(impl_->host_, impl_->resource_);

		// Create a WebSocket session by transferring the socket
		auto session = std::make_shared< ws_client_session >(
			std::move(ws), *this, impl_->websocket_ping_time_);

		session->start();

		impl_->session_ = std::move(session);

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
		return static_cast< bool >(impl_->session_.lock());
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

	void ws_client_base::stop()noexcept{
		impl_->ioc_.stop();
	}


	void ws_client_base::async(std::function< void() > fn){
		boost::asio::defer(impl_->ioc_, [this, fn = std::move(fn)]()noexcept{
				try{
					fn();
				}catch(...){
					on_exception(std::current_exception());
				}
			});
	}


	void ws_client_base::on_open(){}

	void ws_client_base::on_close(){}

	void ws_client_base::on_text(
		boost::beast::multi_buffer const& /*buffer*/){}

	void ws_client_base::on_binary(
		boost::beast::multi_buffer const& /*buffer*/){}

	void ws_client_base::on_error(
		ws_client_location /*location*/,
		boost::system::error_code /*ec*/){}

	void ws_client_base::on_exception(std::exception_ptr /*error*/)noexcept{}


}
