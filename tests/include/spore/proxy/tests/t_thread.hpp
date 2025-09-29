#pragma once

#include "spore/proxy/proxy.hpp"

namespace spore::proxies::tests::threads
{
    template <typename dispatch_t>
    struct facade : proxy_facade<facade<dispatch_t>>
    {
        using dispatch_type = dispatch_t;

        template <std::size_t value_v>
        std::size_t act() const
        {
            constexpr auto func = [](auto& self) { return self.template act<value_v>(); };
            return proxies::dispatch<std::size_t>(*this, func);
        }
    };

    struct impl
    {
        template <std::size_t value_v>
        std::size_t act() const
        {
            return value_v;
        }
    };

    namespace detail
    {
        consteval std::size_t make_random(const std::size_t seed, const std::size_t round)
        {
            std::size_t value = seed + round * 0x9e3779b97f4a7c15;
            value ^= value >> 12;
            value ^= value << 25;
            value ^= value >> 27;
            return value;
        }

        template <std::size_t size_v, std::size_t seed_v>
        consteval std::array<std::size_t, size_v> make_shuffled_array()
        {
            std::array<std::size_t, size_v> indices {};

            for (std::size_t index = 0; index < size_v; ++index)
            {
                indices[index] = index;
            }

            for (std::size_t index = size_v - 1, round = 0; index > 0; --index, ++round)
            {
                const std::size_t rand = make_random(seed_v, round);
                const std::size_t new_index = rand % (index + 1);
                std::swap(indices[index], indices[new_index]);
            }

            return indices;
        }

        template <std::array values_v, std::size_t... indices_v>
        consteval auto make_shuffled_index_sequence(std::index_sequence<indices_v...>)
        {
            return std::index_sequence<values_v[indices_v]...> {};
        }

        template <std::size_t... indices_v>
        consteval auto make_random_sequence(std::index_sequence<indices_v...>)
        {
            return std::integer_sequence<std::size_t, make_random(indices_v, indices_v)...> {};
        }
    }

    template <std::size_t size_v, std::size_t seed_v>
    consteval auto make_shuffled_index_sequence()
    {
        constexpr auto array = detail::make_shuffled_array<size_v, seed_v>();
        return detail::make_shuffled_index_sequence<array>(std::make_index_sequence<size_v> {});
    }

    template <std::size_t size_v>
    consteval auto make_random_sequence()
    {
        return detail::make_random_sequence(std::make_index_sequence<size_v>());
    }
}