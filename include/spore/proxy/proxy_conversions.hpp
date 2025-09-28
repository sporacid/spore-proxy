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

        template <any_proxy_facade facade_t, any_proxy_facade other_facade_t>
        struct semantics_conversion_matrix
        {
            using value_t = proxy_value_semantics<facade_t>;
            using pointer_t = proxy_pointer_semantics<facade_t>;
            using const_pointer_t = proxy_pointer_semantics<const facade_t>;
            using reference_t = proxy_reference_semantics<facade_t&>;
            using temp_reference_t = proxy_reference_semantics<facade_t&&>;
            using const_reference_t = proxy_reference_semantics<const facade_t&>;
            using const_temp_reference_t = proxy_reference_semantics<const facade_t&&>;

            using other_value_t = proxy_value_semantics<other_facade_t>;
            using other_pointer_t = proxy_pointer_semantics<other_facade_t>;
            using other_const_pointer_t = proxy_pointer_semantics<const other_facade_t>;
            using other_reference_t = proxy_reference_semantics<other_facade_t&>;
            using other_temp_reference_t = proxy_reference_semantics<other_facade_t&&>;
            using other_const_reference_t = proxy_reference_semantics<const other_facade_t&>;
            using other_const_temp_reference_t = proxy_reference_semantics<const other_facade_t&&>;

            // clang-format off

            template <any_proxy_semantics semantics_t, any_proxy_semantics other_semantics_t>
            static consteval auto traits_impl(std::in_place_type_t<semantics_t>, std::in_place_type_t<other_semantics_t>)                   -> disable_copy_and_move { return {}; };

            // value semantics conversions

            static consteval auto traits_impl(std::in_place_type_t<value_t>, std::in_place_type_t<other_value_t>)                           -> enable_copy_and_move { return {}; }
            static consteval auto traits_impl(std::in_place_type_t<value_t>, std::in_place_type_t<other_pointer_t>)                         -> enable_copy { return {}; }
            static consteval auto traits_impl(std::in_place_type_t<value_t>, std::in_place_type_t<other_const_pointer_t>)                   -> enable_copy { return {}; }
            static consteval auto traits_impl(std::in_place_type_t<value_t>, std::in_place_type_t<other_reference_t>)                       -> enable_copy { return {}; }
            static consteval auto traits_impl(std::in_place_type_t<value_t>, std::in_place_type_t<other_const_reference_t>)                 -> enable_copy { return {}; }
            static consteval auto traits_impl(std::in_place_type_t<value_t>, std::in_place_type_t<other_temp_reference_t>)                  -> enable_copy_and_move { return {}; }
            static consteval auto traits_impl(std::in_place_type_t<value_t>, std::in_place_type_t<other_const_temp_reference_t>)            -> enable_copy_and_move { return {}; }

            // pointer semantics conversions

            static consteval auto traits_impl(std::in_place_type_t<pointer_t>, std::in_place_type_t<other_pointer_t>)                       -> enable_copy { return {}; };
            static consteval auto traits_impl(std::in_place_type_t<const_pointer_t>, std::in_place_type_t<other_const_pointer_t>)           -> enable_copy { return {}; };
            static consteval auto traits_impl(std::in_place_type_t<const_pointer_t>, std::in_place_type_t<other_pointer_t>)                 -> enable_copy { return {}; };
            static consteval auto traits_impl(std::in_place_type_t<pointer_t>, std::in_place_type_t<other_value_t>)                         -> enable_copy_and_move { return {}; };
            static consteval auto traits_impl(std::in_place_type_t<const_pointer_t>, std::in_place_type_t<other_value_t>)                   -> enable_copy_and_move { return {}; };
            static consteval auto traits_impl(std::in_place_type_t<pointer_t>, std::in_place_type_t<other_reference_t>)                     -> enable_copy { return {}; };
            static consteval auto traits_impl(std::in_place_type_t<pointer_t>, std::in_place_type_t<other_const_reference_t>)               -> enable_copy { return {}; };
            static consteval auto traits_impl(std::in_place_type_t<pointer_t>, std::in_place_type_t<other_temp_reference_t>)                -> enable_copy_and_move { return {}; };
            static consteval auto traits_impl(std::in_place_type_t<pointer_t>, std::in_place_type_t<other_const_temp_reference_t>)          -> enable_copy_and_move { return {}; };

            // reference semantics conversions

            static consteval auto traits_impl(std::in_place_type_t<reference_t>, std::in_place_type_t<other_reference_t>)                   -> enable_copy { return {}; }
            static consteval auto traits_impl(std::in_place_type_t<reference_t>, std::in_place_type_t<other_const_reference_t>)             -> enable_copy { return {}; }
            static consteval auto traits_impl(std::in_place_type_t<reference_t>, std::in_place_type_t<other_temp_reference_t>)              -> enable_copy_and_move { return {}; }
            static consteval auto traits_impl(std::in_place_type_t<reference_t>, std::in_place_type_t<other_const_temp_reference_t>)        -> enable_copy_and_move { return {}; }
            static consteval auto traits_impl(std::in_place_type_t<temp_reference_t>, std::in_place_type_t<other_reference_t>)              -> enable_copy { return {}; }
            static consteval auto traits_impl(std::in_place_type_t<temp_reference_t>, std::in_place_type_t<other_const_reference_t>)        -> enable_copy { return {}; }
            static consteval auto traits_impl(std::in_place_type_t<temp_reference_t>, std::in_place_type_t<other_temp_reference_t>)         -> enable_copy_and_move { return {}; }
            static consteval auto traits_impl(std::in_place_type_t<temp_reference_t>, std::in_place_type_t<other_const_temp_reference_t>)   -> enable_copy_and_move { return {}; }
            static consteval auto traits_impl(std::in_place_type_t<reference_t>, std::in_place_type_t<other_value_t>)                       -> enable_copy_and_move { return {}; }
            static consteval auto traits_impl(std::in_place_type_t<reference_t>, std::in_place_type_t<other_pointer_t>)                     -> enable_copy { return {}; }
            static consteval auto traits_impl(std::in_place_type_t<reference_t>, std::in_place_type_t<other_const_pointer_t>)               -> enable_copy { return {}; }
            static consteval auto traits_impl(std::in_place_type_t<temp_reference_t>, std::in_place_type_t<other_value_t>)                  -> enable_copy_and_move { return {}; }
            static consteval auto traits_impl(std::in_place_type_t<temp_reference_t>, std::in_place_type_t<other_pointer_t>)                -> enable_copy { return {}; }
            static consteval auto traits_impl(std::in_place_type_t<temp_reference_t>, std::in_place_type_t<other_const_pointer_t>)          -> enable_copy { return {}; }

            template <any_proxy_semantics semantics_t, any_proxy_semantics other_semantics_t>
            using traits = decltype(semantics_conversion_matrix::traits_impl(std::in_place_type<semantics_t>, std::in_place_type<other_semantics_t>));
            // clang-format on
        };
    }

    template <any_proxy_facade facade_t, any_proxy_storage storage_t, any_proxy_semantics semantics_t>
    struct proxy;

    template <any_proxy_semantics semantics_t, any_proxy_semantics other_semantics_t>
    struct proxy_semantics_conversion
    {
        using conversion_matrix_t = proxies::detail::semantics_conversion_matrix<typename semantics_t::facade_type, typename other_semantics_t::facade_type>;
        static constexpr bool can_copy = conversion_matrix_t::template traits<semantics_t, other_semantics_t>::can_copy;
        static constexpr bool can_move = conversion_matrix_t::template traits<semantics_t, other_semantics_t>::can_move;
    };

    // clang-format off
    template <any_proxy_facade facade_t, any_proxy_facade other_facade_t>
    struct proxy_facade_conversion
        : std::conditional_t<
            std::derived_from<other_facade_t, facade_t>,
            std::true_type,
            std::false_type
          > {};
    // clang-format on

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
        requires(proxy_facade_conversion<facade_t, other_facade_t>::value)
    struct proxy_conversion<proxy<facade_t, storage_t, semantics_t>, proxy<other_facade_t, other_storage_t, other_semantics_t>>
    {
        static constexpr bool can_copy =
            proxy_semantics_conversion<semantics_t, other_semantics_t>::can_copy and
            proxy_storage_conversion<storage_t, other_storage_t>::can_copy;

        static constexpr bool can_move =
            proxy_semantics_conversion<semantics_t, other_semantics_t>::can_move and
            proxy_storage_conversion<storage_t, other_storage_t>::can_move;
    };

    // clang-format on
}