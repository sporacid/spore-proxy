#pragma once

#include "spore/proxy/proxy_concepts.hpp"
#include "spore/proxy/proxy_facade.hpp"
#include "spore/proxy/proxy_semantics.hpp"
#include "spore/proxy/proxy_storage.hpp"

#include <concepts>

namespace spore
{
    namespace proxies::detail
    {
        struct enable_copy
        {
            static constexpr bool can_copy = true;
            static constexpr bool can_move = false;
        };

        struct enable_copy_and_move
        {
            static constexpr bool can_copy = true;
            static constexpr bool can_move = true;
        };

        struct disable_copy_and_move
        {
            static constexpr bool can_copy = false;
            static constexpr bool can_move = false;
        };
    }

    template <any_proxy_facade facade_t, any_proxy_storage storage_t, any_proxy_semantics semantics_t>
    struct proxy;

    // clang-format off
    template <any_proxy_semantics semantics_t, any_proxy_semantics other_semantics_t>
    struct proxy_semantics_conversion : proxies::detail::disable_copy_and_move {};

    template <any_proxy_facade facade_t>
    struct proxy_semantics_conversion<
        proxy_value_semantics<facade_t>, proxy_value_semantics<facade_t>> : proxies::detail::enable_copy_and_move {};

    template <any_proxy_facade facade_t>
    struct proxy_semantics_conversion<
        proxy_pointer_semantics<facade_t>, proxy_pointer_semantics<facade_t>> : proxies::detail::enable_copy {};

    template <any_proxy_facade facade_t>
    struct proxy_semantics_conversion<
        proxy_pointer_semantics<facade_t>, proxy_pointer_semantics<const facade_t>> : proxies::detail::enable_copy {};

    template <any_proxy_facade facade_t>
    struct proxy_semantics_conversion<
        proxy_pointer_semantics<const facade_t>, proxy_pointer_semantics<const facade_t>> : proxies::detail::enable_copy {};

    template <any_proxy_facade facade_t, any_proxy_facade other_facade_t>
    struct proxy_facade_conversion
        : std::conditional_t<std::derived_from<facade_t, other_facade_t>, std::true_type, std::false_type> {};

    template <any_proxy_storage storage_t, any_proxy_storage other_storage_t>
    struct proxy_storage_conversion
    {
        static constexpr bool can_copy = std::constructible_from<storage_t, const other_storage_t&>;
        static constexpr bool can_move = std::constructible_from<storage_t, other_storage_t&&>;
    };

    template <any_proxy proxy_t, any_proxy other_proxy_t>
    struct proxy_conversion : proxies::detail::disable_copy_and_move
    {
    };

    template <
        any_proxy_facade facade_t, any_proxy_storage storage_t, any_proxy_semantics semantics_t,
        any_proxy_facade other_facade_t, any_proxy_storage other_storage_t, any_proxy_semantics other_semantics_t>
        requires proxy_facade_conversion<facade_t, other_facade_t>::value
    struct proxy_conversion<proxy<facade_t, storage_t, semantics_t>, proxy<other_facade_t, other_storage_t, other_semantics_t>>
    {
        static constexpr bool can_copy = proxy_semantics_conversion<semantics_t, other_semantics_t>::can_copy and proxy_storage_conversion<storage_t, other_storage_t>::can_copy;
        static constexpr bool can_move = proxy_semantics_conversion<semantics_t, other_semantics_t>::can_move and proxy_storage_conversion<storage_t, other_storage_t>::can_move;
    };

    // clang-format on

    //    template <any_proxy_facade facade_t, any_proxy_storage storage_t, any_proxy_semantics semantics_t>
    //    struct proxy;
    //
    //    template <any_proxy proxy_t, any_proxy other_proxy_t>
    //    struct proxy_conversion
    //    {
    //        static constexpr void copy_proxy(proxy_t& proxy, const other_proxy_t& other_proxy) = delete;
    //        static constexpr void move_proxy(proxy_t& proxy, other_proxy_t&& other_proxy) = delete;
    //    };
    //
    //    template <any_proxy proxy_t, any_proxy other_proxy_t>
    //    constexpr bool is_nothrow_proxy_copy_convertible_v = std::is_nothrow_invocable_v<decltype(proxy_conversion<proxy_t, other_proxy_t>::copy_proxy), proxy_t&, const other_proxy_t&>;
    //
    //    template <any_proxy proxy_t, any_proxy other_proxy_t>
    //    constexpr bool is_nothrow_proxy_move_convertible_v = std::is_nothrow_invocable_v<decltype(proxy_conversion<proxy_t, other_proxy_t>::move_proxy), proxy_t&, other_proxy_t&&>;
    //
    //    template <typename proxy_t, typename other_proxy_t>
    //    concept proxy_convertible_to = requires(proxy_t& proxy) {
    //        any_proxy<proxy_t> and any_proxy<other_proxy_t>;
    //        { proxy_conversion<proxy_t, other_proxy_t>::copy_proxy(proxy, std::declval<const other_proxy_t&>()) } -> std::same_as<void>;
    //        { proxy_conversion<proxy_t, other_proxy_t>::move_proxy(proxy, std::declval<other_proxy_t &&>()) } -> std::same_as<void>;
    //    };
}