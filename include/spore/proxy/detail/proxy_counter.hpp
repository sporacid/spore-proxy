#pragma once

namespace spore::proxies::detail
{
    template <unsigned N>
    struct reader
    {
        friend auto counted_flag(reader<N>); // E1
    };

    template <unsigned N>
    struct setter
    {
        friend auto counted_flag(reader<N>) {} // E2

        static constexpr unsigned n = N;
    };

    // E3
    template <
        auto Tag, // E3.1
        unsigned NextVal = 0>
    [[nodiscard]]
    consteval auto counter_impl()
    {
        constexpr bool counted_past_value = requires(reader<NextVal> r)
        {
            counted_flag(r);
        };

        if constexpr (counted_past_value)
        {
            return counter_impl<Tag, NextVal + 1>(); // E3.2
        }
        else
        {
            // E3.3
            setter<NextVal> s;
            return s.n;
        }
    }

    template <
        auto Tag = [] {}, // E4
        auto Val = counter_impl<Tag>()>
    constexpr auto counter = Val;
}