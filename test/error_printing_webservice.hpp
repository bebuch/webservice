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

#include <webservice/websocket_service.hpp>

#include <iostream>


namespace webservice{


	struct error_printing_webservice: webservice::websocket_service{
		void on_accept_error(
			std::uintptr_t,
			std::string const&,
			boost::system::error_code ec
		)override{
			throw boost::system::system_error(ec);
		}

		void on_timer_error(
			std::uintptr_t,
			std::string const&,
			boost::system::error_code ec
		)override{
			throw boost::system::system_error(ec);
		}

		void on_ping_error(
			std::uintptr_t,
			std::string const&,
			boost::system::error_code ec
		)override{
			throw boost::system::system_error(ec);
		}

		void on_read_error(
			std::uintptr_t,
			std::string const&,
			boost::system::error_code ec
		)override{
			throw boost::system::system_error(ec);
		}

		void on_write_error(
			std::uintptr_t,
			std::string const&,
			boost::system::error_code ec
		)override{
			throw boost::system::system_error(ec);
		}

		void on_exception(
			std::uintptr_t,
			std::string const&,
			std::exception_ptr error
		)noexcept override{
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
