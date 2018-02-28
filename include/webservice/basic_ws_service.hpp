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

#include "ws_service_base.hpp"
#include "conversion.hpp"


namespace webservice{


	template <
		typename SendTextType,
		typename SendBinaryType = SendTextType,
		typename ReceiveTextType = SendTextType,
		typename ReceiveBinaryType = SendBinaryType >
	class basic_ws_service: public ws_service_base{
	public:
		static to_const_buffer_t< SendTextType > text_to_const_buffer;

		static to_const_buffer_t< SendBinaryType > binary_to_const_buffer;

		static from_multi_buffer_t< ReceiveTextType > multi_buffer_to_text;

		static from_multi_buffer_t< ReceiveBinaryType > multi_buffer_to_binary;


		using ws_service_base::ws_service_base;


		/// \brief Send a text message to all sessions
		void send_text(SendTextType data){
			auto buffer = text_to_const_buffer(data);
			auto keep_alive = std::make_shared< boost::any >(std::move(data));
			ws_service_base::send_text(buffer, std::move(keep_alive));
		}

		/// \brief Send a text message to session by identifier
		void send_text(std::uintptr_t identifier, SendTextType data){
			auto buffer = text_to_const_buffer(data);
			auto keep_alive = std::make_shared< boost::any >(std::move(data));
			ws_service_base::send_text(
				identifier, buffer, std::move(keep_alive));
		}

		/// \brief Send a text message to all sessions by identifier
		void send_text(
			std::set< std::uintptr_t > const& identifier,
			SendTextType data
		){
			auto buffer = text_to_const_buffer(data);
			auto keep_alive = std::make_shared< boost::any >(std::move(data));
			ws_service_base::send_text(
				identifier, buffer, std::move(keep_alive));
		}


		/// \brief Send a binary message to all sessions
		void send_binary(SendBinaryType data){
			auto buffer = binary_to_const_buffer(data);
			auto keep_alive = std::make_shared< boost::any >(std::move(data));
			ws_service_base::send_binary(buffer, std::move(keep_alive));
		}

		/// \brief Send a binary message to session by identifier
		void send_binary(std::uintptr_t identifier, SendBinaryType data){
			auto buffer = binary_to_const_buffer(data);
			auto keep_alive = std::make_shared< boost::any >(std::move(data));
			ws_service_base::send_binary(
				identifier, buffer, std::move(keep_alive));
		}

		/// \brief Send a binary message to all sessions by identifier
		void send_binary(
			std::set< std::uintptr_t > const& identifier,
			SendBinaryType data
		){
			auto buffer = binary_to_const_buffer(data);
			auto keep_alive = std::make_shared< boost::any >(std::move(data));
			ws_service_base::send_binary(
				identifier, buffer, std::move(keep_alive));
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
		using ws_service_base::send_text;
		using ws_service_base::send_binary;

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
	to_const_buffer_t< SendTextType > basic_ws_service<
		SendTextType,
		SendBinaryType,
		ReceiveTextType,
		ReceiveBinaryType >::text_to_const_buffer;

	template <
		typename SendTextType,
		typename SendBinaryType,
		typename ReceiveTextType,
		typename ReceiveBinaryType >
	to_const_buffer_t< SendBinaryType > basic_ws_service<
		SendTextType,
		SendBinaryType,
		ReceiveTextType,
		ReceiveBinaryType >::binary_to_const_buffer;

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
