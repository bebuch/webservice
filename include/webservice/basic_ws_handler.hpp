//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__basic_ws_handler__hpp_INCLUDED_
#define _webservice__basic_ws_handler__hpp_INCLUDED_

#include "ws_handler_base.hpp"
#include "conversion.hpp"


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
	class basic_ws_handler: public ws_handler_base{
	public:
		static to_shared_const_buffer_t< SendTextType >
			text_to_shared_const_buffer;

		static to_shared_const_buffer_t< SendBinaryType >
			binary_to_shared_const_buffer;

		static from_multi_buffer_t< ReceiveTextType >
			multi_buffer_to_text;

		static from_multi_buffer_t< ReceiveBinaryType >
			multi_buffer_to_binary;


		using ws_handler_base::ws_handler_base;


		/// \brief Send a text message to all sessions
		template < typename SendTextTypeT >
		void send_text(SendTextTypeT&& data){
			ws_handler_base::send_text(
				text_to_shared_const_buffer(
					static_cast< SendTextTypeT&& >(data)));
		}

		/// \brief Send a text message to session
		template < typename SendTextTypeT >
		void send_text(ws_server_session* session, SendTextTypeT&& data){
			ws_handler_base::send_text(
				session, text_to_shared_const_buffer(
					static_cast< SendTextTypeT&& >(data)));
		}


		/// \brief Send a binary message to all sessions
		template < typename SendBinaryTypeT >
		void send_binary(SendBinaryTypeT&& data){
			ws_handler_base::send_binary(
				binary_to_shared_const_buffer(
					static_cast< SendBinaryTypeT&& >(data)));
		}

		/// \brief Send a binary message to session
		template < typename SendBinaryTypeT >
		void send_binary(ws_server_session* session, SendBinaryTypeT&& data){
			ws_handler_base::send_binary(
				session, binary_to_shared_const_buffer(
					static_cast< SendBinaryTypeT&& >(data)));
		}


	private:
		/// \brief Called when a session received a text message
		///
		/// Default implementation does nothing.
		virtual void on_text(
			ws_server_session* /*session*/,
			std::string const& /*resource*/,
			ReceiveTextType&& /*data*/){}

		/// \brief Called when a session received a binary message
		///
		/// Default implementation does nothing.
		virtual void on_binary(
			ws_server_session* /*session*/,
			std::string const& /*resource*/,
			ReceiveBinaryType&& /*data*/){}


		void on_text(
			ws_server_session* session,
			std::string const& resource,
			boost::beast::multi_buffer&& buffer
		)final{
			on_text(session, resource, multi_buffer_to_text(buffer));
		}

		void on_binary(
			ws_server_session* session,
			std::string const& resource,
			boost::beast::multi_buffer&& buffer
		)final{
			on_binary(session, resource, multi_buffer_to_binary(buffer));
		}
	};
#ifdef __clang__
#pragma clang diagnostic pop
#endif


	template <
		typename SendTextType,
		typename SendBinaryType,
		typename ReceiveTextType,
		typename ReceiveBinaryType >
	to_shared_const_buffer_t< SendTextType > basic_ws_handler<
		SendTextType,
		SendBinaryType,
		ReceiveTextType,
		ReceiveBinaryType >::text_to_shared_const_buffer;

	template <
		typename SendTextType,
		typename SendBinaryType,
		typename ReceiveTextType,
		typename ReceiveBinaryType >
	to_shared_const_buffer_t< SendBinaryType > basic_ws_handler<
		SendTextType,
		SendBinaryType,
		ReceiveTextType,
		ReceiveBinaryType >::binary_to_shared_const_buffer;

	template <
		typename SendTextType,
		typename SendBinaryType,
		typename ReceiveTextType,
		typename ReceiveBinaryType >
	from_multi_buffer_t< ReceiveTextType > basic_ws_handler<
		SendTextType,
		SendBinaryType,
		ReceiveTextType,
		ReceiveBinaryType >::multi_buffer_to_text;

	template <
		typename SendTextType,
		typename SendBinaryType,
		typename ReceiveTextType,
		typename ReceiveBinaryType >
	from_multi_buffer_t< ReceiveBinaryType > basic_ws_handler<
		SendTextType,
		SendBinaryType,
		ReceiveTextType,
		ReceiveBinaryType >::multi_buffer_to_binary;



}


#endif
