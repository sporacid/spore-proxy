#pragma once

namespace spore::proxies::detail
{
    struct no_lock
    {
        void lock() noexcept
        {
        }

        void unlock() noexcept
        {
        }
    };
}