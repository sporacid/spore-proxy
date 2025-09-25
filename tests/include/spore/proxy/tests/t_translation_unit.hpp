#pragma once

#include "spore/proxy/proxy.hpp"

namespace spore::proxies::tests::tu
{
    template <typename dispatch_t>
    struct facade : proxy_facade<facade<dispatch_t>>
    {
        using dispatch_type = dispatch_t;

        std::size_t some_work() const
        {
            constexpr auto func = [](auto& self) { return self.some_work(); };
            return proxies::dispatch<std::size_t>(func, *this);
        }

        std::size_t some_other_work() const
        {
            constexpr auto func = [](auto& self) { return self.some_other_work(); };
            return proxies::dispatch<std::size_t>(func, *this);
        }
    };

    template struct facade<proxy_dispatch_dynamic<>>;
    template struct facade<proxy_dispatch_static<>>;

    template <typename dispatch_t>
    shared_proxy<facade<dispatch_t>> make_proxy();

    template <typename dispatch_t>
    std::size_t some_work(const shared_proxy<facade<dispatch_t>>& proxy);

    template <typename dispatch_t>
    std::size_t some_other_work(const shared_proxy<facade<dispatch_t>>& proxy);
}