#pragma once

#include "spore/proxy/proxy.hpp"

namespace spore::proxies::tests
{
    struct translation_unit_facade : proxy_facade<translation_unit_facade>
    {
        template <std::size_t value_v>
        std::size_t act() const
        {
            constexpr auto func = [](auto& self) { return self.template act<value_v>(); };
            return proxies::dispatch<std::size_t>(func, *this);
        }
    };

    shared_proxy<translation_unit_facade> make_proxy();
    std::size_t some_work(const shared_proxy<translation_unit_facade>& proxy);
    std::size_t some_other_work(const shared_proxy<translation_unit_facade>& proxy);
}