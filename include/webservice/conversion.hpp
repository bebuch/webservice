//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__conversion__hpp_INCLUDED_
#define _webservice__conversion__hpp_INCLUDED_

#include <boost/beast/core/multi_buffer.hpp>
#include <boost/beast/core/buffers_to_string.hpp>

#include <boost/asio/buffer.hpp>

#include <type_traits>
#include <utility>


namespace webservice{


	template < typename T, typename = void const* >
	struct has_data_function: std::false_type{};

	template < typename T >
	struct has_data_function< T,
			decltype((void const*)std::declval< T >().data()) >
		: std::true_type{};


	template < typename T, typename = std::size_t >
	struct has_size_function: std::false_type{};

	template < typename T >
	struct has_size_function< T,
			decltype((std::size_t)std::declval< T >().size()) >
		: std::true_type{};


	template < typename T, typename = void const* >
	struct size_of_data_type: std::integral_constant< std::size_t, 0 >{};

	template < typename T >
	struct size_of_data_type< T,
			decltype((void const*)std::declval< T >().data()) >
		: std::integral_constant<
			std::size_t, sizeof(*std::declval< T >().data()) >{};


	template < typename T >
	struct to_const_buffer_t{
		static_assert(
			has_data_function< T >::value &&
			has_size_function< T >::value &&
			size_of_data_type< T >::value == 1,
			"T must have a .data() and a .size() member function and .data() "
			"must return a pointer to a type with sizeof equal 1, "
			"alternatively you can specialize to_const_buffer_t with a "
			"function operator that takes an object of type T and returns "
			"a boost::asio::const_buffer");

		boost::asio::const_buffer operator()(T const& data)const{
			return boost::asio::const_buffer(data.data(), data.size());
		}
	};


	template < typename T, typename = void >
	struct has_append_function: std::false_type{};

	template < typename T >
	struct has_append_function< T,
			decltype((void)std::declval< T >().append(
				std::declval< T >().data(), std::size_t{})) >
		: std::true_type{};


	namespace detail{


		/// \brief C++14 Workaround helper
		template < typename T, typename = void >
		struct call_reserve{
			void operator()(T&, std::size_t s)const{}
		};

		/// \brief Workaround for C++14 which has no non-const data() member
		template < typename T >
		struct call_reserve< T,
			decltype(std::declval< T >().reserve(std::size_t{})) >
		{
			void operator()(T& c, std::size_t s)const{
				c.reserve(s);
			}
		};


	}


	template < typename T >
	struct from_multi_buffer_t{
// 		static_assert(
// 			std::is_default_constructible< T >::value &&
// 			has_data_function< T >::value &&
// 			has_size_function< T >::value &&
// 			size_of_data_type< T >::value == 1 &&
// 			has_append_function< T >::value,
// 			"T must be default constructible and have a .data() and "
// 			"a .size() member function and .data() "
// 			"must return a pointer to a type value_type with sizeof equal 1 "
// 			"and T must have a .append(value_type*, std::size_t) "
// 			"member function, "
// 			"alternatively you can specialize from_multi_buffer_t with a "
// 			"function operator that takes a boost::beast::multi_buffer object "
// 			"returns an object of type T");

		T operator()(boost::beast::multi_buffer const& buffer)const{
			T result;

			detail::call_reserve< T > reserve;
			reserve(result, boost::asio::buffer_size(buffer.data()));

			auto const end = boost::asio::buffer_sequence_end(buffer.data());
			for(
				auto iter = boost::asio::buffer_sequence_begin(buffer.data());
				iter != end; ++iter
			){
				boost::asio::const_buffer buffer(*iter);
				auto const data = reinterpret_cast< typename
						std::remove_pointer< decltype(result.data()) >
							::type const*
					>(buffer.data());
				auto const size = buffer.size();
				result.insert(result.end(), data, data + size);
			}

			return result;
		}
	};


}


#endif
