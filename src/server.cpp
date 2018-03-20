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
#include <mutex>


namespace webservice{


	class server_impl{
	public:
		/// \brief Constructor
		///
		/// Run the IO context on all threads.
		server_impl(
			std::unique_ptr< http_request_handler > handler,
			std::unique_ptr< ws_handler_base > service,
			std::unique_ptr< error_handler > error_handler,
			boost::asio::ip::address const address,
			std::uint16_t const port,
			std::uint8_t const thread_count,
			boost::optional< std::chrono::milliseconds > websocket_ping_time,
			std::size_t max_read_message_size
		)
			: listener_(
				std::move(handler),
				std::move(service),
				std::move(error_handler),
				boost::asio::ip::tcp::endpoint{address, port},
				thread_count,
				websocket_ping_time,
				max_read_message_size)
		{
			// Run the I/O service on the requested number of thread_count
			threads_.reserve(thread_count);
			for(std::size_t i = 0; i < thread_count; ++i){
				threads_.emplace_back([this]{
					// restart io_context if it returned by exception
					for(;;){
						try{
							listener_.run();
							return;
						}catch(...){
							listener_.on_exception(std::current_exception());
						}
					}
				});
			}
		}


		/// \copydoc server::block()
		void block()noexcept{
			std::lock_guard< std::recursive_mutex > lock(mutex);
			for(auto& thread: threads_){
				if(thread.joinable()){
					try{
						thread.join();
					}catch(...){
						listener_.on_exception(std::current_exception());
					}
				}
			}
		}

		/// \copydoc server::stop()
		void stop()noexcept{
			listener_.stop();
		}

		/// \copydoc server::shutdown()
		void shutdown()noexcept{
			listener_.shutdown();
		}


		/// \brief Execute a function async via server threads
		void async(std::function< void() >&& fn){
			listener_.async(std::move(fn));
		}


		/// \brief Run one task in server threads
		void run_one()noexcept{
			listener_.run_one();
		}


	private:
		/// \brief Protect thread joins
		std::recursive_mutex mutex;

		/// \brief The worker threads
		std::vector< std::thread > threads_;

		/// \brief Accepts incoming connections and launches the sessions
		webservice::listener listener_;
	};


	server::server(
		std::unique_ptr< http_request_handler > handler,
		std::unique_ptr< ws_handler_base > service,
		std::unique_ptr< error_handler > error_handler,
		boost::asio::ip::address const address,
		std::uint16_t const port,
		std::uint8_t const thread_count,
		boost::optional< std::chrono::milliseconds > websocket_ping_time,
		std::size_t max_read_message_size
	)
		: impl_(std::make_unique< server_impl >(
				[this, handler = std::move(handler)]()mutable{
						if(!handler){
							handler =
								boost::make_unique< http_request_handler >();
						}
						handler->set_server(*this);
						return std::move(handler);
					}(),
				[this, service = std::move(service)]()mutable{
						if(service){
							service->set_server(*this);
						}
						return std::move(service);
					}(),
				[error_handler = std::move(error_handler)]()mutable{
						if(!error_handler){
							error_handler =
								boost::make_unique< class error_handler >();
						}
						return std::move(error_handler);
					}(),
				address,
				port,
				thread_count,
				websocket_ping_time,
				max_read_message_size
			)) {}

	server::~server(){
		impl_->stop();
		impl_->block();
	}


	void server::block()noexcept{
		impl_->block();
	}

	void server::stop()noexcept{
		impl_->stop();
	}

	void server::shutdown()noexcept{
		impl_->shutdown();
	}


	void server::async(std::function< void() > fn){
		impl_->async(std::move(fn));
	}

	void server::run_one()noexcept{
		impl_->run_one();
	}


}
