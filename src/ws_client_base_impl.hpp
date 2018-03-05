//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__ws_client_base_impl__hpp_INCLUDED_
#define _webservice__ws_client_base_impl__hpp_INCLUDED_

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

#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <mutex>


namespace webservice{


	class ws_client_base_impl{
	public:
		/// \brief Constructor
		ws_client_base_impl(
			ws_client_base& self,
			std::string&& host,
			std::string&& port,
			std::string&& resource,
			boost::optional< std::chrono::milliseconds > websocket_ping_time,
			std::size_t max_read_message_size
		)
			: self_(self)
			, host_(std::move(host))
			, port_(std::move(port))
			, resource_([](std::string&& resource){
					if(resource.empty()) resource = "/";
					return std::move(resource);
				}(std::move(resource)))
			, websocket_ping_time_(websocket_ping_time)
			, max_read_message_size_(max_read_message_size) {}


		/// \brief Connect client to server
		///
		/// Does nothing if client is already connected.
		void connect(){
			std::lock_guard< std::recursive_mutex > lock(mutex_);
			if(is_connected()){
				return;
			}

			block();

			boost::asio::ip::tcp::resolver resolver(ioc_);
			auto results = resolver.resolve(host_, port_);

			ws_stream ws(ioc_);
			ws.read_message_max(max_read_message_size_);

			// Make the connection on the IP address we get from a lookup
			boost::asio::connect(ws.next_layer(),
				results.begin(), results.end());

			// Perform the ws handshake
			ws.handshake(host_, resource_);

			// Create a WebSocket session by transferring the socket
			auto session = std::make_shared< ws_client_session >(
				std::move(ws), *this, websocket_ping_time_);

			session->start();

			session_ = std::move(session);

			// restart io_context if it returned by exception
			thread_ = std::thread([this]{
					for(;;){
						try{
							ioc_.run();
							return;
						}catch(...){
							on_exception(std::current_exception());
						}
					}
				});
		}

		/// \brief true if client is connected to server, false otherwise
		bool is_connected()const{
			return static_cast< bool >(session_.lock());
		}


		/// \brief Send a message
		template < typename Data >
		void send(Data&& data){
			if(auto const session = session_.lock()){
				session->send(static_cast< Data&& >(data));
			}
		}


		/// \brief Wait on the processing threads
		///
		/// This effecivly blocks the current thread until the client is closed.
		void block()noexcept{
			std::lock_guard< std::recursive_mutex > lock(mutex_);
			if(thread_.joinable()){
				try{
					thread_.join();
				}catch(...){
					on_exception(std::current_exception());
				}
			}
		}

		/// \brief Close the connection as fast as possible
		///
		/// This function is not blocking. Call block() if you want to wait
		/// until all connections are closed.
		void stop()noexcept{
			ioc_.stop();
		}


		/// \brief Called when the sessions starts
		void on_open(){
			self_.on_open();
		}

		/// \brief Called when the sessions ends
		void on_close(){
			self_.on_close();
		}

		/// \brief Called when the session received a text message
		void on_text(boost::beast::multi_buffer const& buffer){
			self_.on_text(buffer);
		}

		/// \brief Called when the session received a binary message
		void on_binary(boost::beast::multi_buffer const& buffer){
			self_.on_binary(buffer);
		}

		/// \brief Called when an error occured
		void on_error(
			ws_client_location location,
			boost::system::error_code ec
		){
			self_.on_error(location, ec);
		}

		/// \brief Called when an exception was thrown
		void on_exception(std::exception_ptr error)noexcept{
			self_.on_exception(error);
		}


	private:
		/// \brief Pointer to implementation
		ws_client_base& self_;

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


}


#endif
