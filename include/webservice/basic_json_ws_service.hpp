//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__basic_json_ws_service__hpp_INCLUDED_
#define _webservice__basic_json_ws_service__hpp_INCLUDED_

#include "basic_ws_service.hpp"

#include <nlohmann/json.hpp>


namespace webservice{


	template <
		typename SendBinaryType,
		typename ReceiveBinaryType = SendBinaryType >
	class basic_json_ws_service
		: public basic_ws_service<
			std::string, SendBinaryType, std::string, ReceiveBinaryType >
	{
		using base = basic_ws_service<
			std::string, SendBinaryType, std::string, ReceiveBinaryType >;
	public:
		using basic_ws_service<
				std::string, SendBinaryType, std::string, ReceiveBinaryType
			>::basic_ws_service;


		/// \brief Send a json message to all sessions
		void send_json(nlohmann::json const& data){
			base::send_text(dump(data));
		}

		/// \brief Send a json message to session by identifier
		void send_json(std::uintptr_t identifier, nlohmann::json const& data){
			base::send_text(identifier, dump(data));
		}

		/// \brief Send a json message to all sessions by identifier
		void send_json(
			std::set< std::uintptr_t > const& identifier,
			nlohmann::json const& data
		){
			base::send_text(identifier, dump(data));
		}


	protected:
		/// \brief Called when a session received a json message
		///
		/// Default implementation does nothing.
		virtual void on_json(
			std::uintptr_t /*identifier*/,
			std::string const& /*resource*/,
			nlohmann::json&& /*data*/){}


	private:
		using base::send_text;

		static std::string dump(nlohmann::json const& json){
			try{
				return json.dump();
			}catch(nlohmann::json::exception const& e){
				throw std::runtime_error(std::string(e.what())
					+ "; dump failed");
			}
		}

		void on_text(
			std::uintptr_t identifier,
			std::string const& resource,
			std::string&& data
		)final{
			on_json(identifier, resource, [&data]{
				try{
					return nlohmann::json::parse(data);
				}catch(nlohmann::json::exception const& e){
					throw std::runtime_error(std::string(e.what())
						+ "; parsed expression '" + data + "'");
				}}());
		}
	};


}


#endif
