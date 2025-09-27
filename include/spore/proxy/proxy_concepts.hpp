#pragma once

#include <concepts>
#include <type_traits>

namespace spore
{
    struct proxy_storage_dispatch;

    template <typename storage_t>
    concept any_proxy_storage =
        std::is_same_v<storage_t, std::decay_t<storage_t>> and
        std::is_default_constructible_v<storage_t> and
        requires(storage_t& storage) {
            { storage.ptr() } -> std::same_as<void*>;
            { storage.reset() } -> std::same_as<void>;
            { storage.dispatch() } -> std::same_as<const proxy_storage_dispatch*>;
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

    template <typename facade_t>
    concept any_proxy_facade =
        std::is_same_v<facade_t, std::decay_t<facade_t>> and
        std::is_empty_v<facade_t> and
        std::is_default_constructible_v<facade_t> and
        proxies::detail::is_proxy_facade<facade_t>::value;

    template <typename semantics_t>
    concept any_proxy_semantics =
        std::is_same_v<semantics_t, std::decay_t<semantics_t>> and
        std::is_empty_v<semantics_t> and
        std::is_default_constructible_v<semantics_t> and
        proxies::detail::is_proxy_semantics<semantics_t>::value;

    namespace proxies::detail
    {
        template <typename value_t>
        struct is_proxy : std::false_type
        {
        };
    }

    template <typename proxy_t>
    concept any_proxy =
        std::is_same_v<proxy_t, std::decay_t<proxy_t>> and
        proxies::detail::is_proxy<proxy_t>::value;
}