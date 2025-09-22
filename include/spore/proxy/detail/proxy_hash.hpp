#pragma once

#include <cstddef>
#include <limits>
#include <ranges>
#include <string_view>

namespace spore::proxies::detail
{
    template <typename value_t>
    constexpr std::size_t hash_range(const std::span<const value_t> range)
    {
        constexpr std::size_t initial_hash = static_cast<std::size_t>(14695981039346656037ULL & std::numeric_limits<std::size_t>::max());
        constexpr std::size_t fnv_prime = static_cast<std::size_t>(1099511628211ULL & std::numeric_limits<std::size_t>::max());

        std::size_t hash = initial_hash;

        for (const value_t value : range)
        {
            const std::size_t size = static_cast<std::size_t>(value) & std::numeric_limits<std::size_t>::max();
            hash = (hash ^ size) * fnv_prime;
        }

        return hash;
    }

    constexpr std::size_t hash_string(const std::string_view string)
    {
        return hash_range(std::span {string});
    }

    constexpr std::size_t hash_combine(const std::size_t hash, const std::size_t other_hash)
    {
        return hash ^ (hash + 0x9e3779b9 + (other_hash << 6) + (other_hash >> 2));
    }
}