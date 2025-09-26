#pragma once

#include <concepts>
#include <type_traits>

namespace spore
{
    template <typename storage_t>
    concept any_proxy_storage = requires(storage_t& storage) {
        std::is_default_constructible_v<storage_t>;
        { storage.ptr() } -> std::same_as<void*>;
        { storage.reset() } -> std::same_as<void>;
    };

    template <typename facade_t, typename... facades_t>
    struct proxy_facade;

    namespace proxies::detail
    {
        template <typename facade_t, typename... facades_t>
        consteval std::true_type is_proxy_facade_impl(const proxy_facade<facade_t, facades_t...>&)
        {
            return {};
        }

        consteval std::false_type is_proxy_facade_impl(...)
        {
            return {};
        }

        template <typename value_t>
        struct is_proxy_facade : decltype(is_proxy_facade_impl(std::declval<value_t>())) {};

        template <typename value_t>
        struct is_proxy_semantics : std::false_type
        {
        };
    }

    template <typename value_t>
    concept any_proxy_facade = requires {
        std::is_empty_v<value_t>;
        std::is_default_constructible_v<value_t>;
        proxies::detail::is_proxy_facade<value_t>::value;
    };

    template <typename value_t>
    concept any_proxy_semantics = requires {
        std::is_empty_v<value_t>;
        std::is_default_constructible_v<value_t>;
        proxies::detail::is_proxy_semantics<value_t>::value;
    };

    namespace proxies::detail
    {
        template <typename value_t>
        struct is_proxy : std::false_type
        {
        };
    }

    template <typename value_t>
    concept any_proxy = proxies::detail::is_proxy<value_t>::value;
}