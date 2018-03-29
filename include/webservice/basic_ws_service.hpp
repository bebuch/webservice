//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__basic_ws_service__hpp_INCLUDED_
#define _webservice__basic_ws_service__hpp_INCLUDED_

#include "basic_ws_handler.hpp"

#include <boost/asio/strand.hpp>
#include <boost/asio/post.hpp>

#include <shared_mutex>


namespace webservice{


#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Woverloaded-virtual"
#endif
	template <
		typename SendTextType,
		typename SendBinaryType = SendTextType,
		typename ReceiveTextType = SendTextType,
		typename ReceiveBinaryType = SendBinaryType >
	class basic_ws_service
		: public basic_ws_handler< SendTextType, SendBinaryType,
			ReceiveTextType, ReceiveBinaryType >
	{
		using strand = boost::asio::strand< boost::asio::executor >;
	public:
		using basic_ws_handler< SendTextType, SendBinaryType,
			ReceiveTextType, ReceiveBinaryType >::basic_ws_handler;


		~basic_ws_service(){
			if(!this->server()){
				return;
			}

			while(async_calls_ > 0){
				auto count = this->server()->poll_one();
				(void)count;
				assert(count > 0);
			}
		}


		/// \brief Create a strand with servers executor
		void set_server(class server& server)override{
			std::unique_lock< std::shared_timed_mutex > lock(mutex_);

			if(async_calls_ > 0){
				throw std::logic_error(
					"Can not replace server while async operations are active");
			}

			checked_ws_handler_base::set_server(server);

			if(this->server()){
				strand_ = std::make_unique< strand >(
					this->server()->get_executor());
			}else{
				strand_.reset();
			}
		}


	private:
		/// \brief Called with a unique identifier when a sessions starts
		///
		/// Default implementation does nothing.
		virtual void on_open(std::uintptr_t /*identifier*/){}

		/// \brief Called with a unique identifier when a sessions ends
		///
		/// Default implementation does nothing.
		virtual void on_close(std::uintptr_t /*identifier*/){}

		/// \brief Called when a session received a text message
		///
		/// Default implementation does nothing.
		virtual void on_text(
			std::uintptr_t /*identifier*/,
			ReceiveTextType&& /*data*/){}

		/// \brief Called when a session received a binary message
		///
		/// Default implementation does nothing.
		virtual void on_binary(
			std::uintptr_t /*identifier*/,
			ReceiveBinaryType&& /*data*/){}

		/// \brief Called when an error occured
		///
		/// Default implementation does nothing.
		virtual void on_error(
			std::uintptr_t /*identifier*/,
			ws_handler_location /*location*/,
			boost::system::error_code /*ec*/){}

		/// \brief Called when an exception was thrown
		///
		/// Default implementation does nothing.
		virtual void on_exception(
			std::uintptr_t /*identifier*/,
			std::exception_ptr /*error*/)noexcept{}


		/// \brief Protect strand_
		std::shared_timed_mutex mutex_;

		/// \brief Exec all on_operations async over this strand
		std::unique_ptr< strand > strand_;

		/// \brief Don't destruct while async calls are active
		std::atomic< std::size_t > async_calls_{0};


		/// \brief Called with a unique identifier when a sessions starts
		///
		/// Default implementation does nothing.
		void on_open(
			std::uintptr_t identifier,
			std::string const& /*resource*/
		)final{
			std::shared_lock< std::shared_timed_mutex > lock(mutex_);

			++async_calls_;
			boost::asio::post(boost::asio::bind_executor(
					*strand_,
					[this, identifier]{
						try{
							on_open(identifier);
						}catch(...){
							on_exception(identifier, std::current_exception());
						}
						--async_calls_;
					}
				));
		}

		/// \brief Called with a unique identifier when a sessions ends
		///
		/// Default implementation does nothing.
		void on_close(
			std::uintptr_t identifier,
			std::string const& /*resource*/
		)final{
			std::shared_lock< std::shared_timed_mutex > lock(mutex_);

			++async_calls_;
			boost::asio::post(boost::asio::bind_executor(
					*strand_,
					[this, identifier]{
						try{
							on_close(identifier);
						}catch(...){
							on_exception(identifier, std::current_exception());
						}
						--async_calls_;
					}
				));
		}

		/// \brief Called when a session received a text message
		///
		/// Default implementation does nothing.
		void on_text(
			std::uintptr_t identifier,
			std::string const& /*resource*/,
			ReceiveTextType&& data
		)final{
			std::shared_lock< std::shared_timed_mutex > lock(mutex_);

			++async_calls_;
			boost::asio::post(boost::asio::bind_executor(
					*strand_,
					[this, identifier, data = std::move(data)]()mutable{
						try{
							on_text(identifier, std::move(data));
						}catch(...){
							on_exception(identifier, std::current_exception());
						}
						--async_calls_;
					}
				));
		}

		/// \brief Called when a session received a binary message
		///
		/// Default implementation does nothing.
		void on_binary(
			std::uintptr_t identifier,
			std::string const& /*resource*/,
			ReceiveBinaryType&& data
		)final{
			std::shared_lock< std::shared_timed_mutex > lock(mutex_);

			++async_calls_;
			boost::asio::post(boost::asio::bind_executor(
					*strand_,
					[this, identifier, data = std::move(data)]()mutable{
						try{
							on_binary(identifier, std::move(data));
						}catch(...){
							on_exception(identifier, std::current_exception());
						}
						--async_calls_;
					}
				));
		}

		/// \brief Called when an error occured
		///
		/// Default implementation does nothing.
		void on_error(
			std::uintptr_t identifier,
			std::string const& /*resource*/,
			ws_handler_location location,
			boost::system::error_code ec
		)final{
			std::shared_lock< std::shared_timed_mutex > lock(mutex_);

			++async_calls_;
			boost::asio::post(boost::asio::bind_executor(
					*strand_,
					[this, identifier, location, ec]{
						try{
							on_error(identifier, location, ec);
						}catch(...){
							on_exception(identifier, std::current_exception());
						}
						--async_calls_;
					}
				));
		}

		/// \brief Called when an exception was thrown
		///
		/// Default implementation does nothing.
		void on_exception(
			std::uintptr_t identifier,
			std::string const& /*resource*/,
			std::exception_ptr error
		)noexcept final{
			std::shared_lock< std::shared_timed_mutex > lock(mutex_);

			++async_calls_;
			boost::asio::post(boost::asio::bind_executor(
					*strand_,
					[this, identifier, error]{
						on_exception(identifier, error);
						--async_calls_;
					}
				));
		}
	};
#ifdef __clang__
#pragma clang diagnostic pop
#endif


}


#endif
