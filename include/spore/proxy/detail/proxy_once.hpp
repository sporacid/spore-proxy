#pragma once

// #include <mutex>

namespace spore::proxies::detail
{
    template <typename tag_t>
    struct once
    {
        template <typename func_t>
        once(func_t&& func)
        {
            func();

            // std::call_once(_once_flag, std::forward<func_t>(func));
        }

        once(const once&) = delete;
        once(once&&) = delete;

        once& operator=(const once&) = delete;
        once& operator=(once&&) = delete;

        // private:
        //   static inline std::once_flag _once_flag;
    };
}