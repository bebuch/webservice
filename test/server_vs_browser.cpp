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

#include <boost/make_unique.hpp>

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

boost::beast::string_view to_string(state_t state)noexcept{
	switch(state){
		case state_t::init:         return "init";
		case state_t::file_request: return "file_request";
		case state_t::ws_open:      return "ws_open";
		case state_t::ws_close:     return "ws_close";
		case state_t::ws_text:      return "ws_text";
		case state_t::ws_binary:    return "ws_binary";
		case state_t::exit:         return "exit";
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
	static std::string const test_text;

	void on_open(std::uintptr_t, std::string const&)override{
		check(state_t::ws_open);
		send_text(test_text);
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
		auto const text = boost::beast::buffers_to_string(buffer.data());
		if(test_text == text){
			std::cout << "\033[1;32mpass: '"
				<< test_text
				<< "'\033[0m\n";
		}else{
			std::cout << "\033[1;31mfail: expected '"
				<< test_text << "' but got '" << text
				<< "'\033[0m\n";
		}
		send_binary(std::vector< std::uint8_t >(
			std::begin(test_text), std::end(test_text)));
	}

	void on_binary(
		std::uintptr_t,
		std::string const&,
		boost::beast::multi_buffer& buffer
	)override{
		check(state_t::ws_binary);
		auto const text = boost::beast::buffers_to_string(buffer.data());
		if(test_text == text){
			std::cout << "\033[1;32mpass: '"
				<< test_text
				<< "'\033[0m\n";
		}else{
			std::cout << "\033[1;31mfail: expected '"
				<< test_text << "' but got '" << text
				<< "'\033[0m\n";
		}
		close("shutdown");
	}

	void on_error(
		std::uintptr_t,
		std::string const&,
		boost::system::error_code ec
	)override{
		throw boost::system::system_error(ec);
	}

	void on_exception(
		std::uintptr_t,
		std::string const&,
		std::exception_ptr error
	)noexcept override{
		try{
			std::rethrow_exception(error);
		}catch(std::exception const& e){
			std::cout << "\033[1;31mfail: unexpected exception: "
				<< e.what()
				<< "\033[0m\n";
		}catch(...){
			std::cout
				<< "\033[1;31mfail: unexpected unknown exception\033[0m\n";
		}
	}
};

std::string const websocket_service::test_text = "test text values";


void close_server(int signum){
	std::signal(signum, SIG_DFL);
	std::cout << "Signal: " << signum << '\n';
	std::raise(signum);
}


int main(){
	std::signal(SIGSEGV, &close_server);
	std::signal(SIGABRT, &close_server);
	std::signal(SIGINT, &close_server);

	try{
		auto handler =
			boost::make_unique< file_request_handler >("server_vs_browser");
		auto service = boost::make_unique< websocket_service >();
		webservice::server server(std::move(handler), std::move(service),
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
