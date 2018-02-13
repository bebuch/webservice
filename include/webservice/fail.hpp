//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__fail__hpp_INCLUDED_
#define _webservice__fail__hpp_INCLUDED_

#include <boost/system/error_code.hpp>

#include <boost/utility/string_view.hpp>

#include <iostream>
#include <mutex>


namespace webservice{


	inline std::mutex& log_mutex(){
		static std::mutex mutex;
		return mutex;
	}


	// Report a failure
	inline void log_fail(boost::system::error_code ec, boost::string_view what){
		std::lock_guard< std::mutex > lock(log_mutex());
		std::cerr << what << ": " << ec.message() << "\n";
	}

	// Print an exception
	inline void log_exception(std::exception const& e, boost::string_view pos){
		std::lock_guard< std::mutex > lock(log_mutex());
		std::cerr << "exception in " << pos << ": " << e.what() << "\n";
	}

	// Print an exception
	inline void log_exception(boost::string_view pos){
		std::lock_guard< std::mutex > lock(log_mutex());
		std::cerr << "unknown exception in " << pos << "\n";
	}

	// Print an exception
	inline void log_exception(std::exception_ptr e, boost::string_view pos){
		try{
			std::rethrow_exception(e);
		}catch(std::exception const& e){
			log_exception(e, pos);
		}catch(...){
			log_exception(pos);
		}
	}

	// Print an massage
	inline void log_msg(boost::string_view text){
		std::lock_guard< std::mutex > lock(log_mutex());
		std::cout << "log: " << text << "\n";
	}


}


#endif
