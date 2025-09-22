#pragma once

namespace spore::proxies::detail
{
    struct once
    {
        template <typename func_t>
        once (func_t&& func)
        {
            func();
        }
    };
}