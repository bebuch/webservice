//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__basic_ws_client__hpp_INCLUDED_
#define _webservice__basic_ws_client__hpp_INCLUDED_

#include "ws_client_base.hpp"
#include "conversion.hpp"


namespace webservice{


	template <
		typename SendTextType,
		typename SendBinaryType = SendTextType,
		typename ReceiveTextType = SendTextType,
		typename ReceiveBinaryType = SendBinaryType >
	class basic_ws_client: public ws_client_base{
	public:
		static from_multi_buffer_t< ReceiveTextType > multi_buffer_to_text;

		static from_multi_buffer_t< ReceiveBinaryType > multi_buffer_to_binary;


		using ws_client_base::ws_client_base;


		/// \brief Send a text message
		void send_text(SendTextType data){
			ws_client_base::send_text(shared_const_buffer(std::move(data)));
		}

		/// \brief Send a binary message
		void send_binary(SendBinaryType data){
			ws_client_base::send_binary(shared_const_buffer(std::move(data)));
		}


	protected:
		/// \brief Called when the session received a text message
		///
		/// Default implementation does nothing.
		virtual void on_text(ReceiveTextType&& /*data*/){}

		/// \brief Called when the session received a binary message
		///
		/// Default implementation does nothing.
		virtual void on_binary(ReceiveBinaryType&& /*data*/){}


	private:
		void on_text(boost::beast::multi_buffer const& buffer)final{
			on_text(multi_buffer_to_text(buffer));
		}

		void on_binary(boost::beast::multi_buffer const& buffer)final{
			on_binary(multi_buffer_to_binary(buffer));
		}
	};


	template <
		typename SendTextType,
		typename SendBinaryType,
		typename ReceiveTextType,
		typename ReceiveBinaryType >
	from_multi_buffer_t< ReceiveTextType > basic_ws_client<
		SendTextType,
		SendBinaryType,
		ReceiveTextType,
		ReceiveBinaryType >::multi_buffer_to_text;

	template <
		typename SendTextType,
		typename SendBinaryType,
		typename ReceiveTextType,
		typename ReceiveBinaryType >
	from_multi_buffer_t< ReceiveBinaryType > basic_ws_client<
		SendTextType,
		SendBinaryType,
		ReceiveTextType,
		ReceiveBinaryType >::multi_buffer_to_binary;

}


#endif
