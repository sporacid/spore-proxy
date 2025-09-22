#pragma once

#include "spore/proxy/proxy_macros.hpp"
#include "spore/proxy/proxy_storage.hpp"

#include <type_traits>

namespace spore
{
    template <typename facade_t, typename... facades_t>
    struct proxy_facade;

    namespace proxies::detail
    {
        template <typename facade_t>
        consteval void add_facade();

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
        consteval bool is_proxy_facade()
        {
            return decltype(proxies::detail::is_proxy_facade_impl(std::declval<value_t>()))::value;
        }
    }

    // clang-format off
    template <typename value_t>
    concept any_proxy_facade =
        std::is_empty_v<value_t> and
        std::is_default_constructible_v<value_t> and
        is_proxy_facade<value_t>();
    // clang-format on

    template <typename facade_t, typename... facades_t>
    struct SPORE_PROXY_ENFORCE_EBCO proxy_facade : facades_t...
    {
      private:
        template <typename, any_proxy_storage>
        friend struct proxy;

        template <typename>
        friend struct proxy_view;

        friend facade_t;

        constexpr proxy_facade() noexcept
        {
            // static_assert(any_proxy_facade<facade_t> and (... and any_proxy_facade<facades_t>));
            static_assert(std::is_empty_v<facade_t> and (... and std::is_empty_v<facades_t>));
            static_assert(std::is_default_constructible_v<facade_t> and (... and std::is_default_constructible_v<facades_t>));

            (proxies::detail::add_facade<facade_t>());
            (proxies::detail::add_facade<facades_t>(), ...);
        }
    };
}