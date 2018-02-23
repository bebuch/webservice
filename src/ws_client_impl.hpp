//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__ws_client_impl__hpp_INCLUDED_
#define _webservice__ws_client_impl__hpp_INCLUDED_

#include <webservice/ws_client.hpp>

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


namespace webservice{


	class ws_client_impl{
	public:
		/// \brief Constructor
		ws_client_impl(
			ws_client& self,
			std::string&& host,
			std::string&& port,
			std::string&& resource
		)
			: self_(self)
			, resolver_(ioc_)
		{
			auto results = resolver_.resolve(host, port);

			ws_stream ws(ioc_);

			// Make the connection on the IP address we get from a lookup
			boost::asio::connect(ws.next_layer(),
				results.begin(), results.end());

			// Perform the ws handshake
			ws.handshake(host, resource);

			// Create a WebSocket session by transferring the socket
			auto session = std::make_shared< ws_client_session >(
				std::move(ws), self_);

			session->start();

			session_ = session;

			// restart io_context if it returned by exception
			thread_ = std::thread([this]{
					try{
						self_.on_open();
					}catch(...){
						on_exception(std::current_exception());
					}

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

		/// \brief Destructor
		~ws_client_impl(){
			close();
			if(thread_.joinable()) thread_.join();
		}

		/// \brief Send a message
		template < typename Data >
		void send(Data&& data){
			if(auto const session = session_.lock()){
				session->send(static_cast< Data >(data));
			}
		}

		/// \brief Close the sessions
		void close(){
			ioc_.stop();
		}


		/// \brief Called when the sessions ends
		void on_close(){
			self_.on_close();
		}

		/// \brief Called when the session received a text message
		void on_text(boost::beast::multi_buffer& buffer){
			self_.on_text(buffer);
		}

		/// \brief Called when the session received a binary message
		void on_binary(boost::beast::multi_buffer& buffer){
			self_.on_binary(buffer);
		}

		/// \brief Called when an error occured
		void on_error(
			ws_client_error error,
			boost::system::error_code ec
		){
			self_.on_error(error, ec);
		}

		/// \brief Called when an exception was thrown
		void on_exception(std::exception_ptr error)noexcept{
			self_.on_exception(error);
		}


	private:
		/// \brief Pointer to implementation
		ws_client& self_;

		boost::asio::io_context ioc_;
		boost::asio::ip::tcp::resolver resolver_;
		std::weak_ptr< ws_client_session > session_;
		std::thread thread_;
	};


}


#endif
