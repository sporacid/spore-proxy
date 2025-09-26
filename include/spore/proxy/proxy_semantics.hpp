#pragma once

#include "spore/proxy/proxy_macros.hpp"

namespace spore
{
    template <typename facade_t>
    struct SPORE_PROXY_ENFORCE_EBCO proxy_value_semantics : facade_t
    {
    };

    template <typename facade_t>
    struct SPORE_PROXY_ENFORCE_EBCO proxy_pointer_semantics : private facade_t
    {
        facade_t* operator->() const
        {
            return const_cast<facade_t*>(static_cast<const facade_t*>(this));
        }

        facade_t* operator->()
        {
            return static_cast<facade_t*>(this);
        }

        facade_t& operator*() const
        {
            return *const_cast<facade_t*>(static_cast<const facade_t*>(this));
        }

        facade_t& operator*()
        {
            return *static_cast<facade_t*>(this);
        }
    };

    template <typename facade_t>
    struct SPORE_PROXY_ENFORCE_EBCO proxy_forward_semantics : private std::decay_t<facade_t>
    {
        using facade_reference_type = facade_t&&;
        using facade_pointer_type = std::remove_reference_t<facade_t>*;
        using facade_value_type = std::decay_t<facade_t>;

        std::remove_reference_t<facade_t>* operator->() const
        {
            if constexpr (std::is_const_v<std::remove_reference_t<facade_t>>)
            {
                return const_cast<facade_t*>(static_cast<const facade_t*>(this));
            }
            else
            {
                return static_cast<facade_t*>(this);
            }
        }

        facade_t&& operator*() const
        {
            if constexpr (std::is_const_v<std::remove_reference_t<facade_t>>)
            {
                return std::forward<facade_t>(*const_cast<facade_t*>(static_cast<const facade_t*>(this)));
            }
            else
            {
                return std::forward<facade_t>(*static_cast<facade_t*>(this));
            }
        }
    };

#if 0
    template <typename facade_t>
    struct SPORE_PROXY_ENFORCE_EBCO proxy_pointer_semantics
    {
        facade_t* operator->() const
        {
            return std::addressof(_facade);
        }

        facade_t& operator*() const
        {
            return _facade;
        }

      private:
        SPORE_PROXY_ENFORCE_NO_UNIQUE_ADDRESS mutable facade_t _facade;
    };

    template <typename facade_t>
    struct SPORE_PROXY_ENFORCE_EBCO proxy_forward_semantics
    {
        using facade_reference_type = facade_t&&;
        using facade_pointer_type = std::remove_reference_t<facade_t>*;
        using facade_value_type = std::decay_t<facade_t>;

        facade_pointer_type operator->() const
        {
            return std::addressof(_facade);
        }

        facade_reference_type operator*() const
        {
            return std::forward<facade_t>(_facade);
        }

      private:
        SPORE_PROXY_ENFORCE_NO_UNIQUE_ADDRESS facade_value_type _facade;
    };
#endif

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