//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/http
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include "error_printing_webservice.hpp"
#include "error_printing_error_handler.hpp"
#include "error_printing_request_handler.hpp"

#include <webservice/server.hpp>
#include <webservice/ws_service.hpp>
#include <webservice/ws_client.hpp>

#include <boost/make_unique.hpp>

#include <boost/lexical_cast.hpp>

#include <thread>
#include <random>
#include <csignal>


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


std::vector< std::uint8_t > binary_data;

void fill_data(){
	constexpr std::size_t size = 1024*1024*16;
	std::cout << "begin fill data vector\n";
	binary_data.clear();
	binary_data.reserve(size);
    std::random_device rd;
    std::uniform_int_distribution< std::uint8_t > dist;
	std::cout << "====================\n";
	for(std::size_t i = 0; i < size; ++i){
		binary_data.push_back(dist(rd));
		if(i % (size / 20) == 0){
			std::cout << "-" << std::flush;
		}
	}
	std::cout << "\nend fill data vector" << std::endl;
}

struct ws_service
	: webservice::error_printing_webservice< webservice::ws_service >
{
	std::size_t count = 0;

	void on_open(std::uintptr_t, std::string const&)override{
		std::thread([this]{
				fill_data();
				send_binary(binary_data);
			}).detach();
	}

	void on_close(std::uintptr_t, std::string const&)override{
		server().stop();
	}

	void on_text(
		std::uintptr_t,
		std::string const&,
		std::string&& text
	)override{
		std::cout << "\033[1;31mfail: server unexpected text message '"
			<< text << "'\033[0m\n";
		close("shutdown");
	}

	void on_binary(
		std::uintptr_t,
		std::string const&,
		std::vector< std::uint8_t >&& data
	)override{
		if(data == binary_data){
			std::cout << "\033[1;32mclient pass vector with "
				<< data.size() << "\033[0m\n";
			if(count < 10){
				++count;
				std::thread([this]{
						fill_data();
						send_binary(binary_data);
					}).detach();
			}else{
				close("shutdown");
			}
		}else{
			std::cout << "\033[1;31mfail: client expected vector with size "
				<< binary_data.size() << " but got " << data.size()
				<< " with different data\033[0m\n";
			close("shutdown");
		}
	}
};


struct ws_client: webservice::ws_client{
	using webservice::ws_client::ws_client;

	int count = 0;

	void on_text(std::string&& text)override{
		std::cout << "\033[1;31mfail: client unexpected text message '"
			<< text << "'\033[0m\n";
		close("shutdown");
	}

	void on_binary(std::vector< std::uint8_t >&& data)override{
		if(data == binary_data){
			std::cout << "\033[1;32mclient pass vector with "
				<< data.size() << "\033[0m\n";
			send_binary(binary_data);
		}else{
			std::cout << "\033[1;31mfail: client expected vector with size "
				<< binary_data.size() << " but got " << data.size()
				<< " with different data\033[0m\n";
			close("shutdown");
		}
	}
};


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
		{
			using boost::make_unique;
			webservice::server server(
				make_unique< request_handler >(),
				make_unique< ws_service >(),
				make_unique< webservice::error_printing_error_handler >(),
				boost::asio::ip::make_address("127.0.0.1"), 1234, 1,
				std::chrono::milliseconds(4000));

			ws_client client("127.0.0.1", "1234", "/",
				std::chrono::milliseconds(4000));
			client.connect();

			server.block();
		}

		return 0;
	}catch(std::exception const& e){
		std::cerr << "Exception: " << e.what() << "\n";
		return 1;
	}catch(...){
		std::cerr << "Unknown exception\n";
		return 1;
	}

}
