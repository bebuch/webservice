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

#include "checked_ws_service_base.hpp"
#include "conversion.hpp"


namespace webservice{


	template <
		typename SendTextType,
		typename SendBinaryType = SendTextType,
		typename ReceiveTextType = SendTextType,
		typename ReceiveBinaryType = SendBinaryType >
	class basic_ws_service: public checked_ws_service_base{
	public:
		static to_shared_const_buffer_t< SendTextType >
			text_to_shared_const_buffer;

		static to_shared_const_buffer_t< SendBinaryType >
			binary_to_shared_const_buffer;

		static from_multi_buffer_t< ReceiveTextType >
			multi_buffer_to_text;

		static from_multi_buffer_t< ReceiveBinaryType >
			multi_buffer_to_binary;


		using checked_ws_service_base::checked_ws_service_base;


		/// \brief Send a text message to all sessions
		void send_text(SendTextType data){
			checked_ws_service_base::send_text(
				text_to_shared_const_buffer(std::move(data)));
		}

		/// \brief Send a text message to session by identifier
		void send_text(std::uintptr_t identifier, SendTextType data){
			checked_ws_service_base::send_text(
				identifier, text_to_shared_const_buffer(std::move(data)));
		}

		/// \brief Send a text message to all sessions by identifier
		void send_text(
			std::set< std::uintptr_t > const& identifier,
			SendTextType data
		){
			checked_ws_service_base::send_text(
				identifier, text_to_shared_const_buffer(std::move(data)));
		}


		/// \brief Send a binary message to all sessions
		void send_binary(SendBinaryType data){
			checked_ws_service_base::send_binary(
				binary_to_shared_const_buffer(std::move(data)));
		}

		/// \brief Send a binary message to session by identifier
		void send_binary(std::uintptr_t identifier, SendBinaryType data){
			checked_ws_service_base::send_binary(
				identifier, binary_to_shared_const_buffer(std::move(data)));
		}

		/// \brief Send a binary message to all sessions by identifier
		void send_binary(
			std::set< std::uintptr_t > const& identifier,
			SendBinaryType data
		){
			checked_ws_service_base::send_binary(
				identifier, binary_to_shared_const_buffer(std::move(data)));
		}


	protected:
		/// \brief Called when a session received a text message
		///
		/// Default implementation does nothing.
		virtual void on_text(
			std::uintptr_t /*identifier*/,
			std::string const& /*resource*/,
			ReceiveTextType&& /*data*/){}

		/// \brief Called when a session received a binary message
		///
		/// Default implementation does nothing.
		virtual void on_binary(
			std::uintptr_t /*identifier*/,
			std::string const& /*resource*/,
			ReceiveBinaryType&& /*data*/){}

	private:
		void on_text(
			std::uintptr_t identifier,
			std::string const& resource,
			boost::beast::multi_buffer const& buffer
		)final{
			on_text(identifier, resource, multi_buffer_to_text(buffer));
		}

		void on_binary(
			std::uintptr_t identifier,
			std::string const& resource,
			boost::beast::multi_buffer const& buffer
		)final{
			on_binary(identifier, resource, multi_buffer_to_binary(buffer));
		}
	};


	template <
		typename SendTextType,
		typename SendBinaryType,
		typename ReceiveTextType,
		typename ReceiveBinaryType >
	to_shared_const_buffer_t< SendTextType > basic_ws_service<
		SendTextType,
		SendBinaryType,
		ReceiveTextType,
		ReceiveBinaryType >::text_to_shared_const_buffer;

	template <
		typename SendTextType,
		typename SendBinaryType,
		typename ReceiveTextType,
		typename ReceiveBinaryType >
	to_shared_const_buffer_t< SendBinaryType > basic_ws_service<
		SendTextType,
		SendBinaryType,
		ReceiveTextType,
		ReceiveBinaryType >::binary_to_shared_const_buffer;

	template <
		typename SendTextType,
		typename SendBinaryType,
		typename ReceiveTextType,
		typename ReceiveBinaryType >
	from_multi_buffer_t< ReceiveTextType > basic_ws_service<
		SendTextType,
		SendBinaryType,
		ReceiveTextType,
		ReceiveBinaryType >::multi_buffer_to_text;

	template <
		typename SendTextType,
		typename SendBinaryType,
		typename ReceiveTextType,
		typename ReceiveBinaryType >
	from_multi_buffer_t< ReceiveBinaryType > basic_ws_service<
		SendTextType,
		SendBinaryType,
		ReceiveTextType,
		ReceiveBinaryType >::multi_buffer_to_binary;



}


#endif
