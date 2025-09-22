#pragma once

#include "spore/proxy/proxy.hpp"

namespace spore::proxies::tests::threads
{
    struct facade : proxy_facade<facade>
    {
        template <std::size_t value_v>
        std::size_t act() const
        {
            constexpr auto func = [](auto& self) { return self.template act<value_v>(); };
            return proxies::dispatch<std::size_t>(func, *this);
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
        //        constexpr std::size_t seed = 0x89541939a0560778;
        //        constexpr std::size_t a = 0x26376def3dfa0c0b;
        //        constexpr std::size_t c = 0x27f9fc91d887025c;
        //
        //        constexpr std::size_t next_random(const std::size_t state)
        //        {
        //            return (a * state + c);
        //        }
        //
        //        template <std::size_t index_v>
        //        struct random
        //        {
        //            static constexpr std::size_t value = next_random(random<index_v - 1>::value);
        //        };
        //
        //        template <>
        //        struct random<0>
        //        {
        //            static constexpr std::size_t value = seed;
        //        };

        //        template <std::size_t... Is>
        //        constexpr auto to_array(std::index_sequence<Is...>) {
        //            return std::array<std::size_t, sizeof...(Is)>{Is...};
        //        }
        //
        //        template <std::size_t size_v>
        //        constexpr std::array<std::size_t, size_v> shuffle_array() {
        //            std::array<std::size_t, size_v> arr{};
        //            for (std::size_t i = 0; i < size_v; ++i) arr[i] = i;
        //
        //            // Simple constexpr shuffle (Fisher-Yates)
        //            for (std::size_t i = size_v - 1; i > 0; --i) {
        //                std::size_t j = i % (i + 1); // deterministic fallback
        //                std::swap(arr[i], arr[j]);
        //            }
        //            return arr;
        //        }
        //
        //        // Step 3: Convert array to index_sequence
        //        template <std::array<std::size_t, N> arr, std::size_t... Is>
        //        constexpr auto to_index_sequence_impl(std::index_sequence<Is...>) {
        //            return std::index_sequence<arr[Is]...>{};
        //        }

        //        template <std::size_t iteration_v, std::size_t... indices_v>
        //        auto shuffle_index_sequence(std::index_sequence<indices_v...>)
        //        {
        //        }

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