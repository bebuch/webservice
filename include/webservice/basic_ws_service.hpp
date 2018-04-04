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
		using strand
			= boost::asio::strand< boost::asio::io_context::executor_type >;
	public:
		using basic_ws_handler< SendTextType, SendBinaryType,
			ReceiveTextType, ReceiveBinaryType >::basic_ws_handler;


	private:
		/// \brief Called when a sessions starts
		///
		/// Default implementation does nothing.
		virtual void on_open(ws_server_session* /*session*/){}

		/// \brief Called when a sessions ends
		///
		/// Default implementation does nothing.
		virtual void on_close(ws_server_session* /*session*/){}

		/// \brief Called when a session received a text message
		///
		/// Default implementation does nothing.
		virtual void on_text(
			ws_server_session* /*session*/,
			ReceiveTextType&& /*data*/){}

		/// \brief Called when a session received a binary message
		///
		/// Default implementation does nothing.
		virtual void on_binary(
			ws_server_session* /*session*/,
			ReceiveBinaryType&& /*data*/){}

		/// \brief Called when an error occured
		///
		/// Default implementation does nothing.
		virtual void on_error(
			ws_server_session* /*session*/,
			ws_handler_location /*location*/,
			boost::system::error_code /*ec*/){}

		/// \brief Called when an exception was thrown
		///
		/// Default implementation does nothing.
		virtual void on_exception(
			ws_server_session* /*session*/,
			std::exception_ptr /*error*/)noexcept{}


		/// \brief Called when a sessions starts
		///
		/// Default implementation does nothing.
		void on_open(
			ws_server_session* session,
			std::string const& /*resource*/
		)final{
			try{
				on_open(session);
			}catch(...){
				on_exception(session, std::current_exception());
			}
		}

		/// \brief Called when a sessions ends
		///
		/// Default implementation does nothing.
		void on_close(
			ws_server_session* session,
			std::string const& /*resource*/
		)final{
			try{
				on_close(session);
			}catch(...){
				on_exception(session, std::current_exception());
			}
		}

		/// \brief Called when a session received a text message
		///
		/// Default implementation does nothing.
		void on_text(
			ws_server_session* session,
			std::string const& /*resource*/,
			ReceiveTextType&& data
		)final{
			try{
				on_text(session, std::move(data));
			}catch(...){
				on_exception(session, std::current_exception());
			}
		}

		/// \brief Called when a session received a binary message
		///
		/// Default implementation does nothing.
		void on_binary(
			ws_server_session* session,
			std::string const& /*resource*/,
			ReceiveBinaryType&& data
		)final{
			try{
				on_binary(session, std::move(data));
			}catch(...){
				on_exception(session, std::current_exception());
			}
		}

		/// \brief Called when an error occured
		///
		/// Default implementation does nothing.
		void on_error(
			ws_server_session* session,
			std::string const& /*resource*/,
			ws_handler_location location,
			boost::system::error_code ec
		)final{
			try{
				on_error(session, location, ec);
			}catch(...){
				on_exception(session, std::current_exception());
			}
		}

		/// \brief Called when an exception was thrown
		///
		/// Default implementation does nothing.
		void on_exception(
			ws_server_session* session,
			std::string const& /*resource*/,
			std::exception_ptr error
		)noexcept final{
			on_exception(session, error);
		}
	};
#ifdef __clang__
#pragma clang diagnostic pop
#endif


}


#endif
