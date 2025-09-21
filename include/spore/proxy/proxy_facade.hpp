#pragma once

#include "spore/proxy/proxy_dispatch.hpp"
#include "spore/proxy/proxy_storage.hpp"

#include <type_traits>

namespace spore
{
    template <typename facade_t, typename... facades_t>
    struct proxy_facade;

    namespace detail
    {
        template <typename facade_t, typename... facades_t>
        consteval std::true_type is_proxy_facade(const proxy_facade<facade_t, facades_t...>&)
        {
            return {};
        }

        consteval std::false_type is_proxy_facade(...)
        {
            return {};
        }
    }

    // clang-format off
    template <typename value_t>
    concept any_proxy_facade = decltype(detail::is_proxy_facade(std::declval<value_t>()))::value;
    // clang-format on

    template <typename facade_t, typename... facades_t>
    struct proxy_facade : facades_t...
    {
      protected:
        template <any_proxy_facade, any_proxy_storage>
        friend struct proxy;

        template <any_proxy_facade>
        friend struct proxy_view;

        constexpr proxy_facade() noexcept
        {
            static_assert(std::is_empty_v<facade_t> and (... and std::is_empty_v<facades_t>));
            static_assert(std::is_default_constructible_v<facade_t> and (... and std::is_default_constructible_v<facades_t>));

            (proxies::detail::type_sets::emplace<proxies::detail::base_tag<facade_t>, facades_t>(), ...);
        }
    };
}