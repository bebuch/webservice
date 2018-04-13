//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__async_lock__hpp_INCLUDED_
#define _webservice__async_lock__hpp_INCLUDED_

#include <atomic>
#include <utility>

#include <iostream>
#include <iomanip>
#include <mutex>


namespace webservice{


	/// \brief Count async operations and calls a user defined function after
	///        all async operations have returned
	class async_locker{
	public:
		/// \brief Increase a counter on construction and decrese on destruction
		class lock{
		public:
			static std::mutex mutex;
			static std::atomic< std::size_t > counter;

			/// \brief Increase the counter
			///
			/// \throw std::runtime_error If no other async operation is still
			///                           running
			lock(async_locker& locker, char const* op)
				: locker_(&locker)
				, op_(op)
			{
				count_ = counter++;

				// lock_count_ must be increast by 1 before first usage. After
				// that it must be decreased by 1. This makes sure that if
				// lock_count_ is 0 the on_last_async() was already fired and
				// no new calles are accepted
				if(locker_->lock_count_ != 0){
					throw std::runtime_error("async call after shutdown");
				}

				// If the call was valid increas the counter
				++locker_->lock_count_;

	// 			std::lock_guard< std::mutex > lock(mutex);
	// 				std::cout
	// 					<< std::setw(8) << (count_) << " 1 "
	// 					<< "0x" << std::setfill('0') << std::hex << std::setw(16)
	// 						<< reinterpret_cast< std::size_t >(locker_) << " - "
	// 					<< std::dec << std::setfill(' ') << op_ << std::endl;
			}

			lock(lock const&) = delete;

			/// \brief Move ownership of the lock
			lock(lock&& other)noexcept
				: locker_(other.locker_.exchange(nullptr,
					std::memory_order_relaxed))
				, count_(other.count_)
				, op_(other.op_) {}

			/// \brief Move ownership of the lock
			lock& operator=(lock&& other)noexcept{
				locker_ = other.locker_.exchange(nullptr,
					std::memory_order_relaxed);
				count_ = other.count_;
				op_ = other.op_;
			}

			/// \brief Call unlock()
			~lock(){
				unlock();
			}

			lock& operator=(lock const&) = delete;

			/// \brief Decrese counter, call callback if count becomes 0
			void unlock()noexcept{
				if(auto locker = locker_.exchange(nullptr,
					std::memory_order_relaxed)
				){
					if(--locker->lock_count_ == 0){
						std::lock_guard< std::mutex > lock(mutex);
						std::cout
							<< std::setw(8) << (count_) << " 0 "
							<< "0x" << std::setfill('0') << std::hex << std::setw(16)
								<< reinterpret_cast< std::size_t >(locker) << " - "
							<< std::dec << std::setfill(' ') << op_ << std::endl;
					}

	// 				std::lock_guard< std::mutex > lock(mutex);
	// 				std::cout
	// 					<< std::setw(8) << (count_) << " 3 "
	// 					<< "0x" << std::setfill('0') << std::hex << std::setw(16)
	// 						<< reinterpret_cast< std::size_t >(locker) << " - "
	// 					<< std::dec << std::setfill(' ') << op_ << std::endl;
				}
			}

			/// \brief true is unlock was not called, false otherwise
			bool is_locked()const noexcept{
				return locker_ != nullptr;
			}

			void enter()const{
	// 			std::lock_guard< std::mutex > lock(mutex);
	// 			std::cout
	// 				<< std::setw(8) << (count_) << " 2 "
	// 				<< "0x" << std::setfill('0') << std::hex << std::setw(16)
	// 					<< reinterpret_cast< std::size_t >(locker_) << " - "
	// 				<< std::dec << std::setfill(' ') << op_ << std::endl;
			}


		private:
			/// \brief Pointer to locked object
			std::atomic< async_locker* > locker_;
			std::size_t count_;
			char const* op_;
		};


		/// \brief Construct with a callback function that is called when the
		///        last async operation returns
		template < typename Fn >
		async_locker(Fn&& fn)
			: fn(static_cast< Fn >(fn))
		{
			static_assert(noexcept(fn()), "fn must be a noexcept function");
		}

		/// \brief Generate the first lock object
		///
		/// \throw std::logic_error If first_lock() was called more than one
		///                         time
		/// \throw std::runtime_error If no other async operation is still
		///                           running
		lock make_first_lock(char const* op){
			// set lock_count_ to 1 if it was 0 only
			if(!lock_count_.compare_exchange_strong(0, 1,
				std::memory_order_relaxed, std::memory_order_relaxed)
			){
				throw std::logic_error(
					"async_locker::first_lock() called after first lock.");
			}

			lock result(lock_count_, op);

			// undo the first increase from compare_exchange_strong
			--lock_count_;

			return result;
		}

		/// \brief Generate a lock object
		///
		/// \throw std::runtime_error If first_lock() has not been called
		///                           before or no other async operation is
		///                           still running
		lock make_lock(char const* op){
			return lock(lock_count_, op);
		}

		/// \brief Current count of running async operations
		std::size_t count()const noexcept{
			return lock_count_;
		}


	private:
		/// \brief Called when the last async operation returned
		std::function< void() > on_last_async_callback_;

		/// \brief Counter of async operations
		///
		/// Inreased on every lock()/first_lock() construction, decreased on
		/// every destruction.
		std::atomic< std::size_t > lock_count_{0};
	}


}


#endif
