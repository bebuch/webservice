//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/http
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include "error_printing_ws_service.hpp"
#include "error_printing_error_handler.hpp"
#include "error_printing_request_handler.hpp"
#include "error_printing_ws_client.hpp"

#include <webservice/server.hpp>
#include <webservice/ws_service_handler.hpp>
#include <webservice/json_ws_service.hpp>
#include <webservice/json_ws_client.hpp>

#include <thread>
#include <csignal>


enum class state_t{
	init,
	ws_server_open,
	ws_server_close,
	ws_server_json,
	ws_server_binary,
	ws_client_open,
	ws_client_close,
	ws_client_json,
	ws_client_binary,
	exit
};

boost::beast::string_view to_string(state_t state)noexcept{
	switch(state){
		case state_t::init:             return "init";
		case state_t::ws_server_open:   return "ws_server_open";
		case state_t::ws_server_close:  return "ws_server_close";
		case state_t::ws_server_json:   return "ws_server_json";
		case state_t::ws_server_binary: return "ws_server_binary";
		case state_t::ws_client_open:   return "ws_client_open";
		case state_t::ws_client_close:  return "ws_client_close";
		case state_t::ws_client_json:   return "ws_client_json";
		case state_t::ws_client_binary: return "ws_client_binary";
		case state_t::exit:             return "exit";
	}
	return "unknown state";
}

void pass(state_t expect, state_t got){
	if(expect == got){
		std::cout << "\033[1;32mpass: "
			<< to_string(got)
			<< "\033[0m\n";
	}else{
		std::cout << "\033[1;31mfail: expected "
			<< to_string(expect) << " but got " << to_string(got)
			<< "\033[0m\n";
	}
}

state_t state = state_t::init;

void check(state_t got){
	switch(state){
		case state_t::init:
			pass(state, got);
			state = state_t::ws_server_open;
			break;
		case state_t::ws_server_open:
			pass(state, got);
			state = state_t::ws_client_open;
			break;
		case state_t::ws_client_open:
			pass(state, got);
			state = state_t::ws_client_json;
			break;
		case state_t::ws_client_json:
			pass(state, got);
			state = state_t::ws_server_json;
			break;
		case state_t::ws_server_json:
			pass(state, got);
			state = state_t::ws_client_binary;
			break;
		case state_t::ws_client_binary:
			pass(state, got);
			state = state_t::ws_server_binary;
			break;
		case state_t::ws_server_binary:
			pass(state, got);
			state = state_t::ws_client_close;
			break;
		case state_t::ws_client_close:
			pass(state, got);
			state = state_t::ws_server_close;
			break;
		case state_t::ws_server_close:
			pass(state, got);
			state = state_t::exit;
			break;
		case state_t::exit:
			pass(state, got);
			break;
	}
}


struct request_handler
	: webservice::error_printing_request_handler<
		webservice::http_request_handler >
{
	using error_printing_request_handler::error_printing_request_handler;

	void operator()(
		webservice::http_request&& req,
		webservice::http_response&& send
	)override{
		std::cout << "\033[1;31mfail: unexpected file request '"
			<< req.target() << "'\033[0m\n";
		webservice::http_request_handler::operator()(
			std::move(req), std::move(send));
	}
};


std::string const test_text = "{\"key\":\"value\"}";

struct ws_service_handler: webservice::ws_service_handler{
	void on_error(
		webservice::ws_server_session*,
		std::string const&,
		webservice::ws_handler_location location,
		boost::system::error_code ec
	)override{
		throw boost::system::system_error(ec,
			"location " + std::string(to_string_view(location)));
	}

	void on_exception(
		webservice::ws_server_session*,
		std::string const&,
		std::exception_ptr error
	)noexcept override{
		try{
			std::rethrow_exception(error);
		}catch(std::exception const& e){
			std::cout << "\033[1;31mfail ws_handler: unexpected exception: "
				<< e.what() << "\033[0m\n";
		}catch(...){
			std::cout << "\033[1;31mfail ws_handler: unexpected unknown "
				"exception\033[0m\n";
		}
	}

	void on_unknown_service(
		webservice::ws_server_session*,
		std::string const& resource
	)override{
		throw std::runtime_error("unknown service: " + resource);
	}
};

struct ws_service
	: webservice::error_printing_ws_service< webservice::json_ws_service >
{
	void on_open(std::uintptr_t)override{
		check(state_t::ws_server_open);
		send_text(nlohmann::json::parse(test_text));
	}

	void on_close(std::uintptr_t)override{
		check(state_t::ws_server_close);
	}

	void on_text(
		std::uintptr_t,
		nlohmann::json&& text
	)override{
		check(state_t::ws_server_json);
		if(nlohmann::json::parse(test_text) == text){
			std::cout << "\033[1;32mserver pass: '"
				<< test_text
				<< "'\033[0m\n";
		}else{
			std::cout << "\033[1;31mfail: server expected '"
				<< test_text << "' but got '" << text.dump()
				<< "'\033[0m\n";
		}
		send_binary(std::vector< std::uint8_t >(
			std::begin(test_text), std::end(test_text)));
	}

	void on_binary(
		std::uintptr_t,
		std::vector< std::uint8_t >&& data
	)override{
		check(state_t::ws_server_binary);
		std::string text(data.begin(), data.end());
		if(test_text == text){
			std::cout << "\033[1;32mserver pass: '"
				<< test_text
				<< "'\033[0m\n";
		}else{
			std::cout << "\033[1;31mfail: server expected '"
				<< test_text << "' but got '" << text
				<< "'\033[0m\n";
		}
		close("shutdown");
	}
};


struct ws_client
	: webservice::error_printing_ws_client< webservice::json_ws_client >
{
	using error_printing_ws_client::error_printing_ws_client;

	void on_open()override{
		check(state_t::ws_client_open);
	}

	void on_close()override{
		check(state_t::ws_client_close);
	}

	void on_text(nlohmann::json&& text)override{
		check(state_t::ws_client_json);
		if(nlohmann::json::parse(test_text) == text){
			std::cout << "\033[1;32mclient pass: '"
				<< test_text
				<< "'\033[0m\n";
		}else{
			std::cout << "\033[1;31mfail: client expected '"
				<< test_text << "' but got '" << text
				<< "'\033[0m\n";
		}
		send_text(nlohmann::json::parse(test_text));
	}

	void on_binary(std::vector< std::uint8_t >&& data)override{
		check(state_t::ws_client_binary);
		std::string text(data.begin(), data.end());
		if(test_text == text){
			std::cout << "\033[1;32mclient pass: '"
				<< test_text
				<< "'\033[0m\n";
		}else{
			std::cout << "\033[1;31mfail: client expected '"
				<< test_text << "' but got '" << text
				<< "'\033[0m\n";
		}
		send_binary(std::vector< std::uint8_t >(
			std::begin(test_text), std::end(test_text)));
	}
};


void signal_handler(int signum){
	std::signal(signum, SIG_DFL);
	std::cout << "Signal: " << signum << '\n';
	std::raise(signum);
}


int main(){
	std::signal(SIGSEGV, &signal_handler);
	std::signal(SIGABRT, &signal_handler);
	std::signal(SIGINT, &signal_handler);

	try{
		{
			using std::make_unique;
			auto ws_handler_ptr = make_unique< ws_service_handler >();
			auto& ws_handler = *ws_handler_ptr;
			webservice::server server(
				make_unique< request_handler >(),
				std::move(ws_handler_ptr),
				make_unique< webservice::error_printing_error_handler >(),
				boost::asio::ip::make_address("127.0.0.1"), 1234, 1);

			ws_handler.add_service("/", make_unique< ws_service >());

			check(state_t::init);

			ws_client client("127.0.0.1", "1234", "/");
			client.connect();

			using system_clock = std::chrono::system_clock;
			auto const start = system_clock::now();
			while(
				state != state_t::exit &&
				system_clock::now() < start + std::chrono::seconds(10)
			){
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
		}

		check(state_t::exit);

		return 0;
	}catch(std::exception const& e){
		std::cerr << "Exception: " << e.what() << "\n";
		return 1;
	}catch(...){
		std::cerr << "Unknown exception\n";
		return 1;
	}

}
