//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <webservice/server.hpp>

#include "listener.hpp"

#include <thread>


namespace webservice{


	class server_impl{
	public:
		/// \brief Constructor
		///
		/// Run the IO context on all threads.
		server_impl(
			std::unique_ptr< http_request_handler > handler,
			std::unique_ptr< websocket_service > service,
			boost::asio::ip::address const address,
			std::uint16_t const port,
			std::uint8_t const thread_count,
			server::exception_handler&& handle_exception
		)
			: handle_exception_(std::move(handle_exception))
			, ioc_{thread_count}
			, listener_(
				std::move(handler),
				std::move(service),
				ioc_,
				boost::asio::ip::tcp::endpoint{address, port})
		{
			// Run the I/O service on the requested number of thread_count
			threads_.reserve(thread_count);
			for(std::size_t i = 0; i < thread_count; ++i){
				threads_.emplace_back([this]{
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
				});
			}
		}


		/// \brief Wait until all thread have finished
		~server_impl(){
			stop();
			block();
		}


		/// \copydoc server::block()
		void block(){
			std::lock_guard lock(mutex);
			for(auto& thread: threads_){
				if(thread.joinable()) thread.join();
			}
		}

		/// \copydoc server::stop()
		void stop(){
			ioc_.stop();
		}


	private:
		/// \brief Protect thread joins
		std::recursive_mutex mutex;

		/// \brief Callback that is called if an exception is thrown
		server::exception_handler const handle_exception_;

		/// \brief The worker threads
		std::vector< std::thread > threads_;

		/// \brief The io_context is required for all I/O
		boost::asio::io_context ioc_;

		/// \brief Accepts incoming connections and launches the sessions
		webservice::listener listener_;
	};


	server::server(
		std::unique_ptr< http_request_handler > handler,
		std::unique_ptr< websocket_service > service,
		boost::asio::ip::address const address,
		std::uint16_t const port,
		std::uint8_t const thread_count,
		exception_handler handle_exception
	)
		: impl_(std::make_unique< server_impl >(
				[this, handler = std::move(handler)]()mutable{
						if(!handler){
							handler =
								boost::make_unique< http_request_handler >();
						}
						handler->server_ = this;
						return std::move(handler);
					}(),
				[this, service = std::move(service)]()mutable{
						if(!service){
							service =
								boost::make_unique< websocket_service >();
						}
						service->server_ = this;
						return std::move(service);
					}(),
				address,
				port,
				thread_count,
				std::move(handle_exception)
			)) {}

	server::~server() = default;


	void server::block(){
		impl_->block();
	}

	void server::stop(){
		impl_->stop();
	}


}
