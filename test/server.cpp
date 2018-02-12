//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/http
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <webserver/file_request_handler.hpp>
#include <webserver/fail.hpp>
#include <webserver/server.hpp>

#include <boost/lexical_cast.hpp>

#include <algorithm>
#include <iostream>
#include <csignal>


webserver::server* server = nullptr;

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

		webserver::file_request_handler handler(doc_root);
		webserver::websocket_service service;
		webserver::server server(handler, service, address, port, thread_count);

		::server = &server;
		std::signal(SIGSEGV, &close_server);
		std::signal(SIGABRT, &close_server);
		std::signal(SIGINT, &close_server);

		server.block();

		return 0;
	}catch(std::exception const& e){
		webserver::log_exception(e, "program");
		print_help(argv[0]);
		return 2;
	}catch(...){
		webserver::log_exception("program");
		print_help(argv[0]);
		return 2;
	}

}
