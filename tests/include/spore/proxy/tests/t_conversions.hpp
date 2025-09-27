#pragma once

#include "spore/proxy/proxy.hpp"

namespace spore::proxies::tests::conversions
{
    // clang-format off
    struct base : proxy_facade<base> {};
    struct facade : proxy_facade<facade, base> {};
    struct impl {};
    // clang-format on

//    struct storage
//    {
//        bool copied = false;
//        bool moved = false;
//
//        template <typename value_t, typename... args_t>
//        explicit storage(std::in_place_type_t<value_t>, args_t&&...) {}
//
//        storage(const storage&)
//        {
//            copied = true;
//        }
//
//        storage(storage&&)
//        {
//            moved = true;
//        }
//    };
//
//    template <typename facade_t>
//    using value_proxy = proxy<std::decay_t<facade_t>, proxies::tests::conversions::storage, proxy_value_semantics<facade_t>>;
//
//    template <typename facade_t>
//    using pointer_proxy = proxy<std::decay_t<facade_t>, proxies::tests::conversions::storage, proxy_pointer_semantics<facade_t>>;
//
//    template <typename facade_t>
//    using reference_proxy = proxy<std::decay_t<facade_t>, proxies::tests::conversions::storage, proxy_reference_semantics<facade_t>>;

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

//    template <any_proxy proxy_t, any_proxy other_proxy_t, copy copy_v, move move_v>
//    static void test_conversion()
//    {
//        static_assert(static_cast<bool>(copy_v) == proxy_conversion<proxy_t, other_proxy_t>::can_copy);
//        static_assert(static_cast<bool>(move_v) == proxy_conversion<proxy_t, other_proxy_t>::can_move);
//
//        if constexpr (proxy_conversion<proxy_t, other_proxy_t>::can_copy)
//        {
//            proxy_t proxy {impl {}};
//            other_proxy_t other_proxy = proxy;
//            REQUIRE(other_proxy._storage.copied);
//        }
//
//        if constexpr (proxy_conversion<proxy_t, other_proxy_t>::can_move)
//        {
//            proxy_t proxy {impl {}};
//            other_proxy_t other_proxy = std::move(proxy);
//            REQUIRE(other_proxy._storage.moved);
//        }
//    }
}