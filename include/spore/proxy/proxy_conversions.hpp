#pragma once

#include "spore/proxy/proxy_concepts.hpp"
#include "spore/proxy/proxy_facade.hpp"
#include "spore/proxy/proxy_forward_like.hpp"
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

#if 0
        template <any_proxy_facade facade_t, any_proxy_facade other_facade_t>
        struct semantics_conversion_matrix_2
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

            static consteval auto traits_impl(std::in_place_type_t<value_t>, std::in_place_type_t<other_value_t>)                           -> enable_copy { return {}; }
            static consteval auto traits_impl(std::in_place_type_t<value_t>, std::in_place_type_t<other_value_t&>)                          -> enable_copy { return {}; }
            static consteval auto traits_impl(std::in_place_type_t<value_t>, std::in_place_type_t<other_value_t&&>)                         -> enable_copy_and_move { return {}; }
            static consteval auto traits_impl(std::in_place_type_t<value_t>, std::in_place_type_t<const other_value_t>)                     -> enable_copy { return {}; }
            static consteval auto traits_impl(std::in_place_type_t<value_t>, std::in_place_type_t<const other_value_t&>)                    -> enable_copy { return {}; }
            static consteval auto traits_impl(std::in_place_type_t<value_t>, std::in_place_type_t<const other_value_t&&>)                   -> enable_copy_and_move { return {}; }

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

            static consteval auto traits_impl(std::in_place_type_t<pointer_t>, std::in_place_type_t<other_value_t>)                         -> enable_copy { return {}; }
            static consteval auto traits_impl(std::in_place_type_t<pointer_t>, std::in_place_type_t<other_value_t&>)                        -> enable_copy { return {}; }
            static consteval auto traits_impl(std::in_place_type_t<pointer_t>, std::in_place_type_t<other_value_t&&>)                       -> enable_copy_and_move { return {}; }
            static consteval auto traits_impl(std::in_place_type_t<pointer_t>, std::in_place_type_t<const other_value_t>)                   -> enable_copy { return {}; }
            static consteval auto traits_impl(std::in_place_type_t<pointer_t>, std::in_place_type_t<const other_value_t&>)                  -> enable_copy { return {}; }
            static consteval auto traits_impl(std::in_place_type_t<pointer_t>, std::in_place_type_t<const other_value_t&&>)                 -> enable_copy_and_move { return {}; }

            static consteval auto traits_impl(std::in_place_type_t<const_pointer_t>, std::in_place_type_t<other_value_t>)                   -> enable_copy { return {}; }
            static consteval auto traits_impl(std::in_place_type_t<const_pointer_t>, std::in_place_type_t<other_value_t&>)                  -> enable_copy { return {}; }
            static consteval auto traits_impl(std::in_place_type_t<const_pointer_t>, std::in_place_type_t<other_value_t&&>)                 -> enable_copy_and_move { return {}; }
            static consteval auto traits_impl(std::in_place_type_t<const_pointer_t>, std::in_place_type_t<const other_value_t>)             -> enable_copy { return {}; }
            static consteval auto traits_impl(std::in_place_type_t<const_pointer_t>, std::in_place_type_t<const other_value_t&>)            -> enable_copy { return {}; }
            static consteval auto traits_impl(std::in_place_type_t<const_pointer_t>, std::in_place_type_t<const other_value_t&&>)           -> enable_copy_and_move { return {}; }

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

            template <any_proxy_semantics semantics_t, typename other_semantics_t>
                requires(any_proxy_semantics<std::decay_t<other_semantics_t>>)
            using traits = decltype(semantics_conversion_matrix_2::traits_impl(std::in_place_type<semantics_t>, std::in_place_type<other_semantics_t>));
            // clang-format on
        };
#endif
    }

    template <any_proxy_facade facade_t, any_proxy_storage storage_t, any_proxy_semantics semantics_t>
    struct proxy;

#if 0
    template <any_proxy_semantics semantics_t, typename other_semantics_t>
        requires(any_proxy_semantics<std::decay_t<other_semantics_t>>)
    struct proxy_semantics_conversion_2
    {
        using conversion_matrix_t = proxies::detail::semantics_conversion_matrix_2<typename semantics_t::facade_type, typename std::decay_t<other_semantics_t>::facade_type>;
        static constexpr bool can_copy = conversion_matrix_t::template traits<semantics_t, other_semantics_t>::can_copy;
        static constexpr bool can_move = conversion_matrix_t::template traits<semantics_t, other_semantics_t>::can_move;
    };
#endif

    template <any_proxy_semantics semantics_t, any_proxy_semantics other_semantics_t>
    struct proxy_semantics_conversion
    {
        using conversion_matrix_t = proxies::detail::semantics_conversion_matrix<typename semantics_t::facade_type, typename other_semantics_t::facade_type>;
        static constexpr bool can_copy = conversion_matrix_t::template traits<semantics_t, other_semantics_t>::can_copy;
        static constexpr bool can_move = conversion_matrix_t::template traits<semantics_t, other_semantics_t>::can_move;
    };

    template <any_proxy_facade facade_t, any_proxy_facade other_facade_t>
    struct proxy_facade_conversion
        : std::conditional_t<
              std::derived_from<other_facade_t, facade_t>,
              std::true_type,
              std::false_type>
    {
    };

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
    struct proxy_conversion<proxy<facade_t, storage_t, semantics_t>, proxy<other_facade_t, other_storage_t, other_semantics_t>>
    {
        static constexpr bool can_copy =
            proxy_facade_conversion<facade_t, other_facade_t>::value and
            proxy_semantics_conversion<semantics_t, other_semantics_t>::can_copy and
            proxy_storage_conversion<storage_t, other_storage_t>::can_copy;

        static constexpr bool can_move =
            proxy_facade_conversion<facade_t, other_facade_t>::value and
            proxy_semantics_conversion<semantics_t, other_semantics_t>::can_move and
            proxy_storage_conversion<storage_t, other_storage_t>::can_move;
    };

#if 0
    template <any_proxy proxy_t, typename other_proxy_t>
        requires(any_proxy<std::decay_t<other_proxy_t>>)
    struct proxy_conversion_v2
    {
        using facade_type = typename proxy_t::facade_type;
        using storage_type = typename proxy_t::storage_type;
        using semantics_type = typename proxy_t::semantics_type;

        using other_facade_type = typename std::decay_t<other_proxy_t>::facade_type;
        using other_storage_type = typename std::decay_t<other_proxy_t>::storage_type;
        using other_semantics_type = decltype(std::forward_like<other_proxy_t&&>(std::declval<typename other_proxy_t::semantics_type>()));

        static constexpr bool can_copy =
            proxy_facade_conversion<facade_type, other_facade_type>::value and
            proxy_semantics_conversion<semantics_type, other_semantics_type>::can_copy and
            proxy_storage_conversion<storage_type, other_storage_type>::can_copy;

        static constexpr bool can_move =
            proxy_facade_conversion<facade_type, other_facade_type>::value and
            proxy_semantics_conversion<semantics_type, other_semantics_type>::can_move and
            proxy_storage_conversion<storage_type, other_storage_type>::can_move;
    };
#endif
}