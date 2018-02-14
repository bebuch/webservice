//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/http
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <webservice/file_request_handler.hpp>
#include <webservice/fail.hpp>
#include <webservice/server.hpp>

#include <thread>
#include <csignal>


enum class state_t{
	init,
	file_request,
	ws_open,
	ws_close,
	ws_text,
	ws_binary,
	exit
};

std::string to_string(state_t state){
	switch(state){
		case state_t::init:         return "init";
		case state_t::file_request: return "file_request";
		case state_t::ws_open:      return "ws_open";
		case state_t::ws_close:     return "ws_close";
		case state_t::ws_text:      return "ws_text";
		case state_t::ws_binary:    return "ws_binary";
		case state_t::exit:         return "exit";
	}
	throw std::logic_error("unknown state");
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
			state = state_t::file_request;
			break;
		case state_t::file_request:
			pass(state, got);
			state = state_t::ws_open;
			break;
		case state_t::ws_open:
			pass(state, got);
			state = state_t::ws_text;
			break;
		case state_t::ws_text:
			pass(state, got);
			state = state_t::ws_binary;
			break;
		case state_t::ws_binary:
			pass(state, got);
			state = state_t::ws_close;
			break;
		case state_t::ws_close:
			pass(state, got);
			state = state_t::exit;
			break;
		case state_t::exit:
			pass(state, got);
			break;
	}
}


struct file_request_handler: webservice::file_request_handler{
	using webservice::file_request_handler::file_request_handler;

	void operator()(
		webservice::http_request&& req,
		webservice::http_response&& send
	)override{
		check(state_t::file_request);
		webservice::file_request_handler::operator()(
			std::move(req), std::move(send));
	}
};


struct websocket_service: webservice::websocket_service{
	void on_open(std::uintptr_t, std::string const&)override{
		check(state_t::ws_open);
	}

	void on_close(std::uintptr_t, std::string const&)override{
		check(state_t::ws_close);
	}

	void on_text(
		std::uintptr_t,
		std::string const&,
		boost::beast::multi_buffer& buffer
	)override{
		check(state_t::ws_text);
		auto text = boost::beast::buffers_to_string(buffer.data());
		std::cout << "text: '" << text << "'\n";
		send_text(std::move(text));
	}

	void on_binary(
		std::uintptr_t,
		std::string const&,
		boost::beast::multi_buffer& buffer
	)override{
		check(state_t::ws_binary);
		auto const text = boost::beast::buffers_to_string(buffer.data());
		std::cout << "binary: '" << text << "'\n";
		send_binary(std::vector< std::uint8_t >(
			std::begin(text), std::end(text)));
		close("shutdown");
	}
};


void close_server(int signum){
	std::signal(signum, SIG_DFL);
	std::cout << "\033[1;31mfail: system signal: " << signum << "\033[0m\n";
	std::raise(signum);
}


int main(){
	std::signal(SIGSEGV, &close_server);
	std::signal(SIGABRT, &close_server);
	std::signal(SIGINT, &close_server);

	try{
		file_request_handler handler("browser_vs_server");
		websocket_service service;
		webservice::server server(handler, service,
			boost::asio::ip::make_address("127.0.0.1"), 1234, 1);

		check(state_t::init);

		std::system("xdg-open http://127.0.0.1:1234");

		auto const start = std::chrono::system_clock::now();
		while(
			state != state_t::exit &&
			std::chrono::system_clock::now() < start + std::chrono::seconds(10)
		){
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		server.stop();

		check(state_t::exit);

		return 0;
	}catch(std::exception const& e){
		webservice::log_exception(e, "program");
		return 1;
	}catch(...){
		webservice::log_exception("program");
		return 1;
	}

}
