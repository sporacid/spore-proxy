#pragma once

#include "spore/proxy/proxy_macros.hpp"

namespace spore
{
    template <typename facade_t>
    struct SPORE_PROXY_ENFORCE_EBCO proxy_value_semantics : facade_t
    {
        // facade_t should be a non-cv qualified, non ref-qualified type
        //   e.g. int

        static_assert(not std::is_reference_v<facade_t>);
        static_assert(not std::is_pointer_v<std::decay_t<facade_t>>);
        static_assert(not std::is_const_v<std::remove_reference_t<facade_t>>);
    };

    template <typename facade_t>
    struct SPORE_PROXY_ENFORCE_EBCO proxy_pointer_semantics : private std::remove_const_t<std::remove_volatile_t<facade_t>>
    {
        // facade_t should be a cv-qualified type, non ref-qualified type
        //   e.g. int, const int, const volatile int

        static_assert(not std::is_reference_v<facade_t>);
        static_assert(not std::is_pointer_v<std::decay_t<facade_t>>);

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
    struct SPORE_PROXY_ENFORCE_EBCO proxy_forward_semantics : private std::decay_t<facade_t>
    {
        // facade_t should be a cv-qualified type, ref-qualified type
        //   e.g. int, const int, int&, int&&, const int&, const int&&, const volatile int&

        static_assert(not std::is_pointer_v<std::decay_t<facade_t>>);

        static constexpr bool is_const = std::is_const_v<std::remove_reference_t<facade_t>>;

        std::remove_reference_t<facade_t>* operator->() const
        {
            if constexpr (is_const)
            {
                return static_cast<std::remove_reference_t<facade_t>*>(this);
            }
            else
            {
                return const_cast<std::remove_reference_t<facade_t>*>(static_cast<const std::remove_reference_t<facade_t>*>(this));
            }
        }

        facade_t&& operator*() const
        {
            return std::forward<facade_t&&>(*operator->());
        }
    };

    template <typename value_t>
    struct is_proxy_semantics : std::false_type
    {
    };

    template <typename facade_t>
    struct is_proxy_semantics<proxy_value_semantics<facade_t>> : std::true_type
    {
    };

    template <typename facade_t>
    struct is_proxy_semantics<proxy_pointer_semantics<facade_t>> : std::true_type
    {
    };

    template <typename facade_t>
    struct is_proxy_semantics<proxy_forward_semantics<facade_t>> : std::true_type
    {
    };

    template <typename value_t>
    concept any_proxy_semantics = is_proxy_semantics<value_t>::value;
}