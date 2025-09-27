#pragma once

#include "spore/proxy/proxy.hpp"

namespace spore::proxies::tests::conversions
{
    static constexpr bool should_copy = true;
    static constexpr bool should_move = true;

    static constexpr bool should_not_copy = false;
    static constexpr bool should_not_move = false;

    template <any_proxy proxy_t, any_proxy other_proxy_t, bool copy_v, bool move_v>
    static consteval void static_assert_conversion()
    {
        static_assert(copy_v == proxy_conversion<proxy_t, other_proxy_t>::can_copy);
        static_assert(move_v == proxy_conversion<proxy_t, other_proxy_t>::can_move);
    }
}