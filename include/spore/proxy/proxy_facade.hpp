#pragma once

#include "spore/proxy/proxy_dispatch.hpp"

#include <type_traits>

namespace spore
{
    template <typename facade_t, typename... facades_t>
    struct proxy_facade : facades_t...
    {
        constexpr proxy_facade() noexcept
        {
            static_assert(std::is_empty_v<facade_t> and (... and std::is_empty_v<facades_t>));
            static_assert(std::is_default_constructible_v<facade_t> and (... and std::is_default_constructible_v<facades_t>));

            (proxies::detail::type_lists::append<proxies::detail::base_tag<facade_t>, facades_t>(), ...);
        }
    };
}