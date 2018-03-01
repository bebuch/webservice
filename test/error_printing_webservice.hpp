//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__error_printing_webservice__hpp_INCLUDED_
#define _webservice__error_printing_webservice__hpp_INCLUDED_

#include <webservice/ws_service.hpp>

#include <iostream>


namespace webservice{


	struct error_printing_webservice: webservice::ws_service{
		void on_error(
			std::uintptr_t,
			std::string const&,
			ws_service_location location,
			boost::system::error_code ec
		)override{
			throw boost::system::system_error(ec,
				"location " + std::string(to_string_view(location)));
		}

		void on_exception(
			std::uintptr_t,
			std::string const&,
			std::exception_ptr error
		)noexcept override{
			try{
				std::rethrow_exception(error);
			}catch(std::exception const& e){
				std::cout << "\033[1;31mfail ws_service: unexpected exception: "
					<< e.what() << "\033[0m\n";
			}catch(...){
				std::cout << "\033[1;31mfail ws_service: unexpected unknown "
					"exception\033[0m\n";
			}
		}
	};


}


#endif
