#pragma once

#include "spore/proxy/proxy.hpp"

namespace spore::proxies::tests::conversions
{
    enum class copy : bool
    {
        disabled = false,
        enabled = true
    };

    enum class move : bool
    {
        disabled = false,
        enabled = true
    };

    template <any_proxy proxy_t, any_proxy other_proxy_t, copy copy_v, move move_v>
    static consteval void static_assert_conversion()
    {
        static_assert(static_cast<bool>(copy_v) == proxy_conversion<proxy_t, other_proxy_t>::can_copy);
        static_assert(static_cast<bool>(move_v) == proxy_conversion<proxy_t, other_proxy_t>::can_move);
    }
}