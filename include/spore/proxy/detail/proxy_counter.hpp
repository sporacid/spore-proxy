#pragma once

#include <cstddef>

namespace spore::proxies::detail::counters
{
    template <typename tag_t, std::size_t index_v>
    struct reader
    {
        friend auto counted_flag(reader<tag_t, index_v>);
    };

    template <typename tag_t, std::size_t index_v>
    struct setter
    {
        friend auto counted_flag(reader<tag_t, index_v>) {}

        static constexpr unsigned value = index_v;
    };

    template <typename tag_t, auto unique_v, std::size_t next_v = 0>
    [[nodiscard]] consteval auto get_impl()
    {
        constexpr bool counted_past_value = requires(reader<tag_t, next_v> reader)
        {
            counted_flag(reader);
        };

        if constexpr (counted_past_value)
        {
            return get_impl<tag_t, unique_v, next_v + 1>();
        }
        else
        {
            return setter<tag_t, next_v> {}.value;
        }
    }

    template <typename tag_t, auto unique_v = [] {}, std::size_t next_v = get_impl<tag_t, unique_v>()>
    constexpr auto get = []() consteval { return next_v; };
}