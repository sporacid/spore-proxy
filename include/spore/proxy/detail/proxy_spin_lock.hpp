#pragma once

#include <atomic>

namespace spore::proxies::detail
{
    struct spin_lock
    {
        void lock() noexcept
        {
            while (_flag.test_and_set(std::memory_order_acquire))
            {
            }
        }

        void unlock() noexcept
        {
            _flag.clear(std::memory_order_release);
        }

      private:
        std::atomic_flag _flag = ATOMIC_FLAG_INIT;
    };
}