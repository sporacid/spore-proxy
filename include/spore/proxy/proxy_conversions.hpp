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

    // value semantics conversions

    template <any_proxy_facade facade_t, any_proxy_facade other_facade_t>
    struct proxy_semantics_conversion<
        proxy_value_semantics<facade_t>,
        proxy_value_semantics<other_facade_t>> : proxies::detail::enable_copy_and_move {};

    // pointer semantics conversions

    template <any_proxy_facade facade_t, any_proxy_facade other_facade_t>
    struct proxy_semantics_conversion<
        proxy_pointer_semantics<facade_t>,
        proxy_pointer_semantics<other_facade_t>> : proxies::detail::enable_copy {};

    template <any_proxy_facade facade_t, any_proxy_facade other_facade_t>
    struct proxy_semantics_conversion<
        proxy_pointer_semantics<const facade_t>,
        proxy_pointer_semantics<other_facade_t>> : proxies::detail::enable_copy {};

    template <any_proxy_facade facade_t, any_proxy_facade other_facade_t>
    struct proxy_semantics_conversion<
        proxy_pointer_semantics<const facade_t>,
        proxy_pointer_semantics<const other_facade_t>> : proxies::detail::enable_copy {};

    template <any_proxy_facade facade_t, any_proxy_facade other_facade_t>
    struct proxy_semantics_conversion<
        proxy_pointer_semantics<facade_t>,
        proxy_value_semantics<other_facade_t>> : proxies::detail::enable_copy_and_move {};

    template <any_proxy_facade facade_t, any_proxy_facade other_facade_t>
    struct proxy_semantics_conversion<
        proxy_pointer_semantics<const facade_t>,
        proxy_value_semantics<other_facade_t>> : proxies::detail::enable_copy_and_move {};

    // facade conversions

    template <any_proxy_facade facade_t, any_proxy_facade other_facade_t>
    struct proxy_facade_conversion
        : std::conditional_t<
            std::derived_from<other_facade_t, facade_t>,
            std::true_type,
            std::false_type
          > {};

    // storage conversions

    template <any_proxy_storage storage_t, any_proxy_storage other_storage_t>
    struct proxy_storage_conversion
    {
        static constexpr bool can_copy = std::constructible_from<storage_t, const other_storage_t&>;
        static constexpr bool can_move = std::constructible_from<storage_t, other_storage_t&&>;
    };

    // proxy conversions

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
}