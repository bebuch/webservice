//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/http
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <webserver/listener.hpp>

#include <thread>


int main(int argc, char* argv[]){
	// Check command line arguments.
	if(argc != 5){
		std::cerr <<
			"Usage: advanced-server <address> <port> <doc_root> <threads>\n" <<
			"Example:\n" <<
			"    advanced-server 0.0.0.0 8080 . 1\n";
		return EXIT_FAILURE;
	}

	auto const address = boost::asio::ip::make_address(argv[1]);
	auto const port = static_cast< unsigned short >(std::atoi(argv[2]));
	std::string const doc_root = argv[3];
	auto const threads = std::max< int >(1, std::atoi(argv[4]));

	// The io_context is required for all I/O
	boost::asio::io_context ioc{threads};

	// Create and launch a listening port
	std::make_shared< webserver::listener >(
		ioc,
		boost::asio::ip::tcp::endpoint{address, port},
		doc_root)->run();

	// Run the I/O service on the requested number of threads
	std::vector< std::thread > v;
	v.reserve(threads - 1);
	for(auto i = threads - 1; i > 0; --i){
		v.emplace_back([&ioc]{ ioc.run(); });
	}
	ioc.run();

	return 0;
}
