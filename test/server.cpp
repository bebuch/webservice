//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/http
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <webserver/listener.hpp>

#include <boost/lexical_cast.hpp>

#include <thread>
#include <algorithm>


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
		std::string const doc_root = argv[3];
		auto const thread_count = std::max< std::uint16_t >(
			1, boost::lexical_cast< std::uint16_t >(argv[4]));

		// Run the I/O service on the requested number of thread_count
		std::vector< std::thread > threads;
		threads.reserve(thread_count);

		// The io_context is required for all I/O
		boost::asio::io_context ioc{thread_count};

		// Create and launch a listening port
		std::make_shared< webserver::listener >(
			ioc,
			boost::asio::ip::tcp::endpoint{address, port},
			doc_root)->run();

		for(std::size_t i = 0; i < thread_count; ++i){
			threads.emplace_back([&ioc]{
				webserver::log_msg("start io_context thread("
					+ boost::lexical_cast< std::string >(
						std::this_thread::get_id()) + ")");
				for(;;){
					try{
						ioc.run();
						break;
					}catch(std::exception& e){
						webserver::log_exception(e, "exception in io_context thread("
							+ boost::lexical_cast< std::string >(
								std::this_thread::get_id()) + ")");
					}catch(...){
						webserver::log_exception("unknown exception in io_context thread("
							+ boost::lexical_cast< std::string >(
								std::this_thread::get_id()) + ")");
					}
					webserver::log_msg("restart io_context thread("
						+ boost::lexical_cast< std::string >(
							std::this_thread::get_id()) + ")");
				}
				webserver::log_msg("exit io_context thread("
					+ boost::lexical_cast< std::string >(
						std::this_thread::get_id()) + ")");
			});
		}

		for(auto& thread: threads){
			thread.join();
		}

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
