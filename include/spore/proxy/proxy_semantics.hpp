#pragma once

#include "spore/proxy/proxy_concepts.hpp"
#include "spore/proxy/proxy_macros.hpp"

#include <type_traits>

namespace spore
{
    template <any_proxy_facade facade_t>
    struct SPORE_PROXY_ENFORCE_EBCO proxy_value_semantics : facade_t
    {
        // facade_t should be a non-cv qualified, non ref-qualified type
        //   e.g. int

        using facade_type = facade_t;
    };

    template <typename facade_t>
        requires(any_proxy_facade<std::remove_const_t<std::remove_volatile_t<facade_t>>>)
    struct SPORE_PROXY_ENFORCE_EBCO proxy_pointer_semantics : private std::remove_const_t<std::remove_volatile_t<facade_t>>
    {
        // facade_t should be a cv-qualified type, non ref-qualified type
        //   e.g. int, const int, const volatile int

        using facade_type = std::remove_const_t<std::remove_volatile_t<facade_t>>;

        static constexpr bool is_const = std::is_const_v<facade_t>;

        facade_t* operator->() const
        {
            if constexpr (is_const)
            {
                return static_cast<facade_t*>(this);
            }
            else
            {
                return const_cast<facade_t*>(static_cast<const facade_t*>(this));
            }
        }

        facade_t& operator*() const
        {
            return *operator->();
        }
    };

    template <typename facade_t>
        requires(any_proxy_facade<std::decay_t<facade_t>> and std::is_reference_v<facade_t>)
    struct SPORE_PROXY_ENFORCE_EBCO proxy_reference_semantics : private std::decay_t<facade_t>
    {
        // facade_t should be a cv-qualified type, ref-qualified type
        //   e.g. int&, int&&, const int&, const int&&, const volatile int&

        using facade_type = std::decay_t<facade_t>;

        static constexpr bool is_const = std::is_const_v<std::remove_reference_t<facade_t>>;

        facade_t&& get() const
        {
            if constexpr (is_const)
            {
                auto* ptr = static_cast<std::remove_reference_t<facade_t>*>(this);
                return std::forward<facade_t&&>(*ptr);
            }
            else
            {
                auto* ptr = const_cast<std::remove_reference_t<facade_t>*>(static_cast<const std::remove_reference_t<facade_t>*>(this));
                return std::forward<facade_t&&>(*ptr);
            }
        }

        facade_t&& operator*() const
        {
            return get();
        }
    };

    namespace proxies::detail
    {
        template <typename facade_t>
        struct is_proxy_semantics<proxy_value_semantics<facade_t>> : std::true_type
        {
        };

        template <typename facade_t>
        struct is_proxy_semantics<proxy_pointer_semantics<facade_t>> : std::true_type
        {
        };

        template <typename facade_t>
        struct is_proxy_semantics<proxy_reference_semantics<facade_t>> : std::true_type
        {
        };

        template <typename semantics_t>
        struct is_proxy_semantics_movable : std::false_type
        {
        };

        template <any_proxy_facade facade_t>
        struct is_proxy_semantics_movable<proxy_value_semantics<facade_t>&&> : std::true_type
        {
        };

        template <any_proxy_facade facade_t>
        struct is_proxy_semantics_movable<proxy_value_semantics<const facade_t>&&> : std::true_type
        {
        };

        template <any_proxy_facade facade_t>
        struct is_proxy_semantics_movable<proxy_reference_semantics<facade_t&&>> : std::true_type
        {
        };

        template <any_proxy_facade facade_t>
        struct is_proxy_semantics_movable<const proxy_reference_semantics<facade_t&&>> : std::true_type
        {
        };

        template <any_proxy_facade facade_t>
        struct is_proxy_semantics_movable<proxy_reference_semantics<facade_t&&>&> : std::true_type
        {
        };

        template <any_proxy_facade facade_t>
        struct is_proxy_semantics_movable<const proxy_reference_semantics<facade_t&&>&> : std::true_type
        {
        };

        template <any_proxy_facade facade_t>
        struct is_proxy_semantics_movable<proxy_reference_semantics<facade_t&&>&&> : std::true_type
        {
        };

        template <any_proxy_facade facade_t>
        struct is_proxy_semantics_movable<const proxy_reference_semantics<facade_t&&>&&> : std::true_type
        {
        };

        template <any_proxy_facade facade_t>
        struct is_proxy_semantics_movable<proxy_reference_semantics<const facade_t&&>> : std::true_type
        {
        };

        template <any_proxy_facade facade_t>
        struct is_proxy_semantics_movable<const proxy_reference_semantics<const facade_t&&>> : std::true_type
        {
        };

        template <any_proxy_facade facade_t>
        struct is_proxy_semantics_movable<proxy_reference_semantics<const facade_t&&>&> : std::true_type
        {
        };

        template <any_proxy_facade facade_t>
        struct is_proxy_semantics_movable<const proxy_reference_semantics<const facade_t&&>&> : std::true_type
        {
        };

        template <any_proxy_facade facade_t>
        struct is_proxy_semantics_movable<proxy_reference_semantics<const facade_t&&>&&> : std::true_type
        {
        };

        template <any_proxy_facade facade_t>
        struct is_proxy_semantics_movable<const proxy_reference_semantics<const facade_t&&>&&> : std::true_type
        {
        };
    }
}