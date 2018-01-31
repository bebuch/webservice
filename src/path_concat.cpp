//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webserver
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <webserver/path_concat.hpp>


namespace webserver{


	std::string path_concat(
		boost::beast::string_view base,
		boost::beast::string_view path
	){
		if(base.empty()){
			return path.to_string();
		}

		std::string result = base.to_string();
		if(result.back() == '/'){
			result.resize(result.size() - 1);
		}

		result.append(path.data(), path.size());
		return result;
	}


}
