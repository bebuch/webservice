//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__websocket_client__hpp_INCLUDED_
#define _webservice__websocket_client__hpp_INCLUDED_

#include <boost/beast/core/multi_buffer.hpp>
#include <boost/beast/core/string.hpp>

#include <memory>
#include <string>
#include <vector>


namespace webservice{


	class websocket_client;

	class websocket_client_impl{
	public:
		/// \brief Constructor
		websocket_client_impl(
			websocket_client& self,
			std::string&& host,
			std::uint16_t port,
			std::string&& resource
		)
			: self_(self)
			, resolver_(ioc_)
			, ws_(ioc_)
			, thread_([
				this,
				host = std::move(host),
				resource = std::move(resource)
			]{
				// Look up the domain name
				resolver_.async_resolve(
					host,
					port,
					[this, host resource]()mutable(
						boost::system::error_code ec,
						tcp::resolver::results_type results
					){
						if(ec){
							return fail(ec, "resolve");
						}

						// Make the connection on the IP address we get from a
						// lookup
						boost::asio::async_connect(
							ws_.next_layer(),
							results.begin(),
							results.end(),
							[
								this,
								host = std::move(host),
								resource = std::move(resource)
							](boost::system::error_code ec){
								if(ec){
									return fail(ec, "connect");
								}

								// Perform the websocket handshake
								ws_.async_handshake(host, resource,
									[](boost::system::error_code ec){
										if(ec){
											on_error(ec);
										}else{
											on_open();
										}
									});
							});
					});

				// restart io_context if it returned by exception
				for(;;){
					try{
						ioc_.run();
						return;
					}catch(...){
						if(handle_exception_){
							try{
								handle_exception_(std::current_exception());
							}catch(std::exception const& e){
								log_exception(e,
									"server::exception_handler");
							}catch(...){
								log_exception("server::exception_handler");
							}
						}else{
							log_exception(std::current_exception(),
								"server::exception");
						}
					}
				}
			}){}

		/// \brief Destructor
		~websocket_client_impl(){
			ioc_.stop();
			if(thread_.joinable()) thread_.join();
		}


		/// \brief Send a message
		void send(boost::asio::buffer data){
			ws_.async_write(
				data,
				[this](
					boost::system::error_code ec,
					std::size_t /*bytes_transferred*/
				){
					if(ec){
						on_error(ec);
					}
				});
		}

		/// \brief Close the sessions
		void close(boost::beast::string_view reason){
			ws_.close(reason);
			ioc_.stop();
		}

		void on_read(boost::system::error_code ec){
			if(ec){
				return fail(ec, "read");
			}

			// Read a message into our buffer
			if(ws_.open()) ws_.async_read(
				buffer_,
				[this](
					boost::system::error_code ec,
					std::size_t /*bytes_transferred*/
				){
					on_read(ec);
				});
		}

		void on_close(boost::system::error_code ec){
			if(ec){
				return fail(ec, "close");
			}
		}


	private:
		/// \brief Called when the sessions starts
		void on_open(){
			self_.on_open();

			ws_.async_read(
				buffer_,
				[this](
					boost::system::error_code ec,
					std::size_t /*bytes_transferred*/
				){
					on_read(ec);
				});
		}

		/// \brief Called when the sessions ends
		void on_close(boost::beast::string_view reason){
			self_.on_open(reason);
		}

		/// \brief Called when the session received a text message
		void on_text(boost::beast::multi_buffer& buffer){
			self_.on_text(buffer);
		}

		/// \brief Called when the session received a binary message
		void on_binary(boost::beast::multi_buffer& buffer){
			self_.on_binary(buffer);
		}


		/// \brief Pointer to implementation
		websocket_client& self_;

		boost::asio::io_context ioc_;
		boost::asio::tcp::resolver resolver_;
		boost::beast::websocket::stream< boost::asio::tcp::socket > ws_;
		boost::asio::strand< boost::asio::io_context::executor_type > strand_;
		boost::beast::multi_buffer buffer_;

		std::thread thread_;
	};


}


#endif
