#pragma once

#include "spore/proxy/proxy.hpp"

namespace spore::proxies::tests::templates
{
    struct facade : proxy_facade<facade>
    {
        template <std::size_t value_v>
        std::size_t act() const
        {
            constexpr auto func = [](auto& self) { return self.template act<value_v>(); };
            return proxies::dispatch<std::size_t>(func, *this);
        }
    };

    struct impl
    {
        template <std::size_t value_v>
        std::size_t act() const
        {
            return value_v;
        }
    };
}