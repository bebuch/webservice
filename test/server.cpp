//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/http
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <webservice/file_request_handler.hpp>
#include <webservice/server.hpp>

#include <boost/lexical_cast.hpp>

#include <boost/make_unique.hpp>

#include <algorithm>
#include <iostream>
#include <csignal>


webservice::server* server = nullptr;

void close_server(int signum){
	std::signal(signum, SIG_DFL);
	std::cout << "Signal: " << signum << '\n';
	server->stop();
	std::raise(signum);
}


void print_help(char const* const exec_name){
	std::cerr << "Usage: " << exec_name
		<< " <address> <port> <doc_root> <thread_count>\n"
		<< "Example:\n"
		<< "    " << exec_name << " 0.0.0.0 8080 . 1\n";
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
		auto const thread_count = std::max< std::uint8_t >(
			1, boost::numeric_cast< std::uint8_t >(
				boost::lexical_cast< unsigned >(argv[4])));
		std::string const doc_root = argv[3];

		using webservice::file_request_handler;
		using webservice::websocket_service;
		using webservice::server;
		server server(
			boost::make_unique< file_request_handler >(doc_root),
			boost::make_unique< websocket_service >(),
			nullptr, address, port, thread_count);

		::server = &server;
		std::signal(SIGSEGV, &close_server);
		std::signal(SIGABRT, &close_server);
		std::signal(SIGINT, &close_server);

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
