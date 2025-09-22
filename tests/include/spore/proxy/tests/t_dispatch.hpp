#pragma once

namespace spore::proxies::tests
{
    // clang-format off
    template <typename value_t>
    concept actable = requires(const value_t& value)
    {
        { value.act() };
    };
    // clang-format on
}