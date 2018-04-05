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


		std::set< ws_identifier > set_intersection(
			std::set< ws_identifier > set1,
			std::set< ws_identifier > set2
		){
			auto first1 = set1.begin();
			auto last1 = set1.end();
			auto first2 = set2.begin();
			auto last2 = set2.end();

			std::set< ws_identifier > result;
			auto d_first = std::inserter(result, result.begin());

			while(first1 != last1 && first2 != last2){
				if(*first1 < *first2){
					++first1;
				}else{
					if(!(*first2 < *first1)){
						*d_first++ = *first1++;
					}
					++first2;
				}
			}

			return result;
		}


	}


	ws_handler_base::ws_handler_base()
		: list_(std::make_unique< ws_sessions >()) {}

	ws_handler_base::~ws_handler_base() = default;


	void ws_handler_base::emplace(
		boost::asio::ip::tcp::socket&& socket,
		http_request&& req
	){
		assert(server() != nullptr);

		ws_stream ws(std::move(socket));
		ws.read_message_max(max_read_message_size());

		auto iter = list_->emplace(std::move(ws), *this, ping_time());
		iter->do_accept(std::move(req));
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

	void ws_handler_base::shutdown()noexcept{}


	void ws_handler_base::send_text(
		ws_identifier identifier,
		shared_const_buffer buffer
	){
		list_->shared_call([this, identifier, &buffer](
			std::set< ws_identifier > const& identifiers
		){
			if(identifiers.count(identifier) == 0){
				return;
			}

			identifier.session
				->send(std::make_tuple(text_tag{}, std::move(buffer)));
		});
	}

	void ws_handler_base::send_text(
		std::set< ws_identifier > const& identifiers,
		shared_const_buffer buffer
	){
		list_->shared_call([this, identifiers, &buffer](
			std::set< ws_identifier > const& valid_identifiers
		){
			auto valids = set_intersection(identifiers, valid_identifiers);
			for(auto identifier: valids){
				identifier.session
					->send(std::make_tuple(text_tag{}, std::move(buffer)));
			}
		});
	}

	void ws_handler_base::send_text(shared_const_buffer buffer){
		list_->shared_call([this, &buffer](
			std::set< ws_identifier > const& identifiers
		){
			for(auto identifier: identifiers){
				identifier.session
					->send(std::make_tuple(text_tag{}, std::move(buffer)));
			}
		});
	}

	void ws_handler_base::send_binary(
		ws_identifier identifier,
		shared_const_buffer buffer
	){
		list_->shared_call([this, identifier, &buffer](
			std::set< ws_identifier > const& identifiers
		){
			if(identifiers.count(identifier) == 0){
				return;
			}

			identifier.session
				->send(std::make_tuple(binary_tag{}, std::move(buffer)));
		});
	}

	void ws_handler_base::send_binary(
		std::set< ws_identifier > const& identifiers,
		shared_const_buffer buffer
	){
		list_->shared_call([this, identifiers, &buffer](
			std::set< ws_identifier > const& valid_identifiers
		){
			auto valids = set_intersection(identifiers, valid_identifiers);
			for(auto identifier: valids){
				identifier.session
					->send(std::make_tuple(binary_tag{}, std::move(buffer)));
			}
		});
	}

	void ws_handler_base::send_binary(shared_const_buffer buffer){
		list_->shared_call([this, &buffer](
			std::set< ws_identifier > const& identifiers
		){
			for(auto identifier: identifiers){
				identifier.session
					->send(std::make_tuple(binary_tag{}, std::move(buffer)));
			}
		});
	}

	void ws_handler_base::close(
		ws_identifier identifier,
		boost::beast::string_view reason
	){
		list_->shared_call([this, identifier, reason](
			std::set< ws_identifier > const& identifiers
		){
			if(identifiers.count(identifier) == 0){
				return;
			}

			identifier.session
				->send(boost::beast::websocket::close_reason(reason));
		});
	}

	void ws_handler_base::close(
		std::set< ws_identifier > const& identifiers,
		boost::beast::string_view reason
	){
		list_->shared_call([this, identifiers, reason](
			std::set< ws_identifier > const& valid_identifiers
		){
			auto valids = set_intersection(identifiers, valid_identifiers);
			for(auto identifier: valids){
				identifier.session
					->send(boost::beast::websocket::close_reason(reason));
			}
		});
	}

	void ws_handler_base::close(boost::beast::string_view reason){
		list_->shared_call([this, reason](
			std::set< ws_identifier > const& identifiers
		){
			for(auto identifier: identifiers){
				identifier.session
					->send(boost::beast::websocket::close_reason(reason));
			}
		});
	}


	void ws_handler_base::set_server(class server& server){
		list_->set_server(server);
	}

	class server* ws_handler_base::server()noexcept{
		return list_->server();
	}


}
