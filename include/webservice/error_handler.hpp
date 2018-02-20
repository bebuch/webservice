//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__error_handler__hpp_INCLUDED_
#define _webservice__error_handler__hpp_INCLUDED_

#include <boost/system/error_code.hpp>

#include <exception>


namespace webservice{


	class listener;


	/// \brief Base class for server error handlers
	class error_handler{
	public:
		/// \brief Destructor
		virtual ~error_handler();

	protected:
		/// \brief Called when an error occured
		///
		/// Default implementation does nothing.
		virtual void on_accept_error(boost::system::error_code ec);

		/// \brief Called when an exception occurred
		///
		/// Default implementation does nothing.
		virtual void on_exception(std::exception_ptr error)noexcept;

		friend class listener;
	};


}


#endif
