#pragma once

#include "spore/proxy/detail/proxy_counter.hpp"

#include <algorithm>
#include <cstddef>

namespace spore::proxies::detail
{
    template <typename...>
    struct type_set
    {
    };

    namespace type_sets
    {
        template <typename tag_t, std::size_t>
        struct nth
        {
            consteval auto friend get(nth);
        };

        template <typename tag_t, std::size_t index_v, typename value_t>
        struct set
        {
            consteval auto friend get(nth<tag_t, index_v>)
            {
                return value_t {};
            }
        };

        template <std::size_t index_v, typename type_set_t>
        struct element_at;

        template <typename value_t, typename... values_t>
        struct element_at<0, type_set<value_t, values_t...>>
        {
            using type = value_t;
        };

        template <std::size_t index_v, typename value_t, typename... values_t>
        struct element_at<index_v, type_set<value_t, values_t...>> : element_at<index_v - 1, type_set<values_t...>>
        {
        };

        template <typename type_set_t>
        struct size_of;

        template <typename... values_t>
        struct size_of<type_set<values_t...>>
        {
            static constexpr auto value = sizeof...(values_t);
        };

        template <typename value_t, typename... values_t>
        consteval auto emplace_impl(type_set<values_t...>)
        {
            if constexpr (pack_contains<value_t, values_t...>())
            {
                return type_set<values_t...> {};
            }
            else
            {
                return type_set<values_t..., value_t> {};
            }
        }

        template <std::size_t index_v, typename type_set_t, typename func_t>
        constexpr void for_each_impl(const func_t& func)
        {
            if constexpr (index_v < size_of<type_set_t>::value)
            {
                using value_t = typename element_at<index_v, type_set_t>::type;
                func.template operator()<value_t>();
                for_each_impl<index_v + 1, type_set_t>(func);
            }
        }

        template <typename tag_t, std::size_t index_v = 0, auto unique_v = [] {}>
        consteval auto get()
        {
            if constexpr (requires { get(nth<tag_t, index_v> {}); })
            {
                return get<tag_t, index_v + 1, unique_v>();
            }
            else if constexpr (index_v == 0)
            {
                return type_set {};
            }
            else
            {
                return get(nth<tag_t, index_v - 1> {});
            }
        }

        template <typename tag_t, typename value_t, std::size_t index_v = 0, auto unique_v = [] {}>
        consteval void emplace()
        {
            if constexpr (requires { get(nth<tag_t, index_v> {}); })
            {
                emplace<tag_t, value_t, index_v + 1, unique_v>();
            }
            else if constexpr (index_v == 0)
            {
                void(set<tag_t, index_v, type_set<value_t>> {});
            }
            else
            {
                void(set<tag_t, index_v, decltype(emplace_impl<value_t>(get(nth<tag_t, index_v - 1> {})))> {});
            }
        }

        template <typename tag_t, typename func_t>
        constexpr void for_each(const func_t& func)
        {
            using type_set_t = decltype(get<tag_t>());
            for_each_impl<0, type_set_t>(func);
        }
    }
}