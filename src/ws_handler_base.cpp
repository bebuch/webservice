//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <webservice/ws_handler_base.hpp>
#include <webservice/server.hpp>

#include "ws_session.hpp"
#include "ws_sessions.hpp"


namespace webservice{


	namespace{


		std::set< ws_server_session* > set_intersection(
			ws_sessions::set const& set1,
			std::set< ws_identifier > const& set2
		){
			auto first1 = set1.begin();
			auto last1 = set1.end();
			auto first2 = set2.begin();
			auto last2 = set2.end();

			std::set< ws_server_session* > result;
			auto d_first = std::inserter(result, result.begin());

			while(first1 != last1 && first2 != last2){
				if(set1.key_comp()(*first1, *first2)){
					++first1;
				}else{
					if(!(set1.key_comp()(*first2, *first1))){
						*d_first++ = (*first1++).get();
					}
					++first2;
				}
			}

			return result;
		}


	}


	ws_handler_base::ws_handler_base() = default;

	ws_handler_base::~ws_handler_base(){
		list_->block();
	}


	void ws_handler_base::async_emplace(
		boost::asio::ip::tcp::socket&& socket,
		http_request&& req
	){
		assert(server() != nullptr);

		ws_stream ws(std::move(socket));
		ws.read_message_max(max_read_message_size());

		list_->async_emplace(std::move(req), std::move(ws), *this, ping_time());
	}

	void ws_handler_base::async_erase(ws_server_session* session){
		list_->async_erase(session);
	}

	void ws_handler_base::on_open(
		ws_identifier /*identifier*/,
		std::string const& /*resource*/){}

	void ws_handler_base::on_close(
		ws_identifier /*identifier*/,
		std::string const& /*resource*/){}

	void ws_handler_base::on_text(
		ws_identifier /*identifier*/,
		std::string const& /*resource*/,
		boost::beast::multi_buffer&& /*buffer*/){}

	void ws_handler_base::on_binary(
		ws_identifier /*identifier*/,
		std::string const& /*resource*/,
		boost::beast::multi_buffer&& /*buffer*/){}

	void ws_handler_base::on_error(
		ws_identifier /*identifier*/,
		std::string const& /*resource*/,
		ws_handler_location /*location*/,
		boost::system::error_code /*ec*/){}

	void ws_handler_base::on_exception(
		ws_identifier /*identifier*/,
		std::string const& /*resource*/,
		std::exception_ptr /*error*/)noexcept{}

	void ws_handler_base::on_shutdown()noexcept{}

	void ws_handler_base::shutdown()noexcept{
		list_->shutdown();
		on_shutdown();
	}

	bool ws_handler_base::is_shutdown()noexcept{
		return list_->is_shutdown();
	}


	void ws_handler_base::send_text(
		ws_identifier identifier,
		shared_const_buffer buffer
	){
		list_->async_call(
			[identifier, buffer = std::move(buffer)](
				ws_sessions::set const& sessions
			)mutable{
				if(sessions.count(identifier) == 0){
					return;
				}

				identifier.session->send(
					std::make_tuple(text_tag{}, std::move(buffer)));
			});
	}

	void ws_handler_base::send_text(
		std::set< ws_identifier > const& identifiers,
		shared_const_buffer buffer
	){
		list_->async_call(
			[identifiers, buffer = std::move(buffer)](
				ws_sessions::set const& sessions
			)mutable{
				auto valid_sessions = set_intersection(sessions, identifiers);
				for(auto session: valid_sessions){
					session->send(
						std::make_tuple(text_tag{}, std::move(buffer)));
				}
			});
	}

	void ws_handler_base::send_text(shared_const_buffer buffer){
		list_->async_call(
			[buffer = std::move(buffer)](
				ws_sessions::set const& sessions
			)mutable{
				for(auto& session: sessions){
					session->send(
						std::make_tuple(text_tag{}, std::move(buffer)));
				}
		});
	}

	void ws_handler_base::send_binary(
		ws_identifier identifier,
		shared_const_buffer buffer
	){
		list_->async_call(
			[identifier, buffer = std::move(buffer)](
				ws_sessions::set const& sessions
			)mutable{
				if(sessions.count(identifier) == 0){
					return;
				}

				identifier.session->send(
					std::make_tuple(binary_tag{}, std::move(buffer)));
			});
	}

	void ws_handler_base::send_binary(
		std::set< ws_identifier > const& identifiers,
		shared_const_buffer buffer
	){
		list_->async_call(
			[identifiers, buffer = std::move(buffer)](
				ws_sessions::set const& sessions
			)mutable{
				auto valid_sessions = set_intersection(sessions, identifiers);
				for(auto session: valid_sessions){
					session->send(
						std::make_tuple(binary_tag{}, std::move(buffer)));
				}
			});
	}

	void ws_handler_base::send_binary(shared_const_buffer buffer){
		list_->async_call(
			[buffer = std::move(buffer)](
				ws_sessions::set const& sessions
			)mutable{
				for(auto& session: sessions){
					session->send(
						std::make_tuple(binary_tag{}, std::move(buffer)));
				}
			});
	}

	void ws_handler_base::close(
		ws_identifier identifier,
		boost::beast::string_view reason
	){
		list_->async_call(
			[identifier, reason](ws_sessions::set const& sessions){
				if(sessions.count(identifier) == 0){
					return;
				}

				identifier.session->send(
					boost::beast::websocket::close_reason(reason));
			});
	}

	void ws_handler_base::close(
		std::set< ws_identifier > const& identifiers,
		boost::beast::string_view reason
	){
		list_->async_call(
			[identifiers, reason](ws_sessions::set const& sessions){
				auto valid_sessions = set_intersection(sessions, identifiers);
				for(auto session: valid_sessions){
					session->send(
						boost::beast::websocket::close_reason(reason));
				}
			});
	}

	void ws_handler_base::close(boost::beast::string_view reason){
		list_->async_call(
			[reason](ws_sessions::set const& sessions){
				for(auto& session: sessions){
					session->send(
						boost::beast::websocket::close_reason(reason));
				}
			});
	}


	void ws_handler_base::set_server(class server& server){
		list_ = std::make_unique< ws_sessions >(server);
	}

	class server* ws_handler_base::server()noexcept{
		return list_->server();
	}


}
