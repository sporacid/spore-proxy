#pragma once

namespace spore::proxies::detail
{
    template <typename tag_t>
    struct once
    {
        template <typename func_t>
        once(func_t&& func)
        {
            func();
        }

        once(const once&) = delete;
        once(once&&) = delete;

        once& operator=(const once&) = delete;
        once& operator=(once&&) = delete;
    };
}