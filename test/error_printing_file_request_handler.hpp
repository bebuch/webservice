//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__error_printing_file_request_handler__hpp_INCLUDED_
#define _webservice__error_printing_file_request_handler__hpp_INCLUDED_

#include <webservice/file_request_handler.hpp>

#include <iostream>


namespace webservice{


	struct error_printing_file_request_handler: file_request_handler{
		using webservice::file_request_handler::file_request_handler;

		void on_error(
			http_request_location,
			boost::system::error_code ec
		)override{
			throw boost::system::system_error(ec);
		}

		void on_exception(std::exception_ptr error)noexcept override{
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


}


#endif
