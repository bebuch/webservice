//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/http
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <webservice/ws_client.hpp>

#include <algorithm>
#include <iostream>
#include <csignal>


struct ws_client: webservice::ws_client{
	using webservice::ws_client::ws_client;

	void on_open()override{
		std::cout << "open session\n";
	}

	void on_close()override{
		std::cout << "closed\n";
	}

	void on_text(std::string&& text)override{
		std::cout << "received text message: " << text << "\n";
	}

	void on_binary(std::vector< std::uint8_t >&& /*data*/)override{
		std::cout << "received binary message\n";
	}
};


ws_client* client = nullptr;

void close_client(int signum){
	std::signal(signum, SIG_DFL);
	std::cout << "Signal: " << signum << '\n';
	client->shutdown();
	client->block();
	std::raise(signum);
}


void print_help(char const* const exec_name){
	std::cerr << "Usage: " << exec_name
		<< " <host> <port> <resource>\n"
		<< "Example:\n"
		<< "    " << exec_name << " host port resource\n";
}


int main(int argc, char* argv[]){
	// Check command line arguments.
	if(argc != 4){
		print_help(argv[0]);
		return 1;
	}

	try{
		auto const host = argv[1];
		auto const port = argv[2];
		auto const resource = argv[3];

		ws_client client(host, port, resource);
		client.connect();

		client.send_text("text from client");

		// Allow to shutdown the client with CTRL+C
		::client = &client;
		std::signal(SIGSEGV, &close_client);
		std::signal(SIGABRT, &close_client);
		std::signal(SIGINT, &close_client);

		client.block();

		return 0;
	}catch(std::exception const& e){
		std::cerr << "Exception: " << e.what() << "\n";
		return 1;
	}catch(...){
		std::cerr << "Unknown exception\n";
		return 1;
	}
}
