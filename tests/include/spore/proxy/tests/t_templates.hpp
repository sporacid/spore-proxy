#pragma once

#include "spore/proxy/proxy.hpp"

namespace spore::proxies::tests::templates
{
    template <typename dispatch_t>
    struct facade : proxy_facade<facade<dispatch_t>>
    {
        using dispatch_type = dispatch_t;

        template <std::size_t value_v>
        std::size_t act() const
        {
            constexpr auto func = [](auto& self) { return self.template act<value_v>(); };
            return proxies::dispatch<std::size_t>(*this, func);
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