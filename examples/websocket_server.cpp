//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/http
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <webservice/file_request_handler.hpp>
#include <webservice/ws_service.hpp>
#include <webservice/server.hpp>

#include <boost/lexical_cast.hpp>

#include <iostream>
#include <csignal>


struct mirror_ws_service: webservice::ws_service{
	void on_open(webservice::ws_identifier identifier)override{
		std::cout << "open session " << identifier << "\n";
	}

	void on_close(webservice::ws_identifier identifier)override{
		std::cout << identifier << " closed\n";
	}

	void on_text(
		webservice::ws_identifier identifier,
		std::string&& text
	)override{
		std::cout << identifier << " received text message: " << text << "\n";

		// Send received text to all WebSocket sessions
		send_text(text);
	}

	void on_binary(
		webservice::ws_identifier identifier,
		std::vector< std::uint8_t >&& data
	)override{
		std::cout << identifier << " received binary message\n";

		// Send received data to all WebSocket sessions
		send_binary(data);
	}
};


webservice::server* server = nullptr;


void on_interrupt(int signum){
	std::signal(signum, SIG_DFL);
	std::cout << "Signal: " << signum << '\n';
	server->shutdown();
	std::cout << "Signal ready\n";
}


void print_help(char const* const exec_name){
	std::cerr << "Usage: " << exec_name
		<< " <address> <port> <doc_root> <thread_count>\n"
		<< "Example:\n"
		<< "    " << exec_name << " 0.0.0.0 8080 http_root_directory 1\n";
}


int main(int argc, char* argv[]){
	// Check command line arguments.
	if(argc != 5){
		print_help(argv[0]);
		return 1;
	}

	try{
		auto const address = boost::asio::ip::make_address(argv[1]);
		auto const port = boost::lexical_cast< std::uint16_t >(argv[2]);
		auto const thread_count = boost::numeric_cast< std::uint8_t >(
			boost::lexical_cast< unsigned >(argv[4]));
		std::string const doc_root = argv[3];

		using webservice::file_request_handler;
		using webservice::server;
		server server(
			std::make_unique< file_request_handler >(doc_root),
			std::make_unique< mirror_ws_service >(),
			nullptr, // ignore errors and exceptions
			address, port, thread_count);

		// Allow to shutdown the server with CTRL+C
		::server = &server;
		std::signal(SIGINT, &on_interrupt);

		server.block();

		return 0;
	}catch(std::exception const& e){
		std::cerr << "Exception: " << e.what() << "\n";
		return 1;
	}catch(...){
		std::cerr << "Unknown exception\n";
		return 1;
	}
}
