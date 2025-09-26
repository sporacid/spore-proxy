#pragma once

#include "spore/proxy/proxy_concepts.hpp"
#include "spore/proxy/proxy_facade.hpp"
#include "spore/proxy/proxy_semantics.hpp"
#include "spore/proxy/proxy_storage.hpp"

#include <concepts>

namespace spore
{
    template <any_proxy_facade facade_t, any_proxy_storage storage_t, any_proxy_semantics semantics_t>
    struct proxy;

    template <any_proxy proxy_t, any_proxy other_proxy_t>
    struct proxy_conversion
    {
        static constexpr void copy_proxy(proxy_t& proxy, const other_proxy_t& other_proxy) = delete;
        static constexpr void move_proxy(proxy_t& proxy, other_proxy_t&& other_proxy) = delete;
    };

    template <any_proxy proxy_t, any_proxy other_proxy_t>
    constexpr bool is_nothrow_proxy_copy_convertible_v = std::is_nothrow_invocable_v<decltype(proxy_conversion<proxy_t, other_proxy_t>::copy_proxy), proxy_t&, const other_proxy_t&>;

    template <any_proxy proxy_t, any_proxy other_proxy_t>
    constexpr bool is_nothrow_proxy_move_convertible_v = std::is_nothrow_invocable_v<decltype(proxy_conversion<proxy_t, other_proxy_t>::move_proxy), proxy_t&, other_proxy_t&&>;

    template <typename proxy_t, typename other_proxy_t>
    concept proxy_convertible_to = requires(proxy_t& proxy) {
        any_proxy<proxy_t> and any_proxy<other_proxy_t>;
        { proxy_conversion<proxy_t, other_proxy_t>::copy_proxy(proxy, std::declval<const other_proxy_t&>()) } -> std::same_as<void>;
        { proxy_conversion<proxy_t, other_proxy_t>::move_proxy(proxy, std::declval<other_proxy_t &&>()) } -> std::same_as<void>;
    };
}