#pragma once

#include <cstddef>

namespace spore::proxies::detail
{
    template <typename...>
    struct type_list
    {
    };

    namespace type_lists
    {
        template <typename tag_t, std::size_t>
        struct nth
        {
            auto friend get(nth);
        };

        template <typename tag_t, std::size_t index_v, typename value_t>
        struct set
        {
            auto friend get(nth<tag_t, index_v>)
            {
                return value_t {};
            }
        };

        template <std::size_t index_v, typename type_list_t>
        struct element_at;

        template <typename value_t, typename... values_t>
        struct element_at<0, type_list<value_t, values_t...>>
        {
            using type = value_t;
        };

        template <std::size_t index_v, typename value_t, typename... values_t>
        struct element_at<index_v, type_list<value_t, values_t...>> : element_at<index_v - 1, type_list<values_t...>>
        {
        };

        template <typename type_list_t>
        struct size_of;

        template <typename... values_t>
        struct size_of<type_list<values_t...>>
        {
            static constexpr auto value = sizeof...(values_t);
        };

        template <typename value_t, typename... values_t>
        consteval auto append_impl(type_list<values_t...>)
        {
            return type_list<values_t..., value_t> {};
        }

        template <std::size_t index_v, typename type_list_t, typename func_t>
        constexpr void for_each_impl(const func_t& func)
        {
            if constexpr (index_v < size_of<type_list_t>::value)
            {
                using value_t = typename element_at<index_v, type_list_t>::type;
                func.template operator()<value_t>();
                for_each_impl<index_v + 1, type_list_t>(func);
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
                return type_list {};
            }
            else
            {
                return get(nth<tag_t, index_v - 1> {});
            }
        }

        template <typename tag_t, typename value_t, std::size_t index_v = 0, auto unique_v = [] {}>
        consteval void append()
        {
            if constexpr (requires { get(nth<tag_t, index_v> {}); })
            {
                append<tag_t, value_t, index_v + 1, unique_v>();
            }
            else if constexpr (index_v == 0)
            {
                void(set<tag_t, index_v, type_list<value_t>> {});
            }
            else
            {
                void(set<tag_t, index_v, decltype(append_impl<value_t>(get(nth<tag_t, index_v - 1> {})))> {});
            }
        }

        template <typename tag_t, typename func_t>
        constexpr void for_each(const func_t& func)
        {
            using type_list_t = decltype(get<tag_t>());
            for_each_impl<0, type_list_t>(func);
        }
    }
}