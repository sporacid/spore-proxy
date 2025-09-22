#pragma once

#include <cstdint>
#include <limits>
#include <source_location>
#include <string_view>

namespace spore::proxies::detail
{
    template <typename value_t>
    consteval std::size_t type_id()
    {
        constexpr auto hash_string = [](const std::string_view value) {
            constexpr std::size_t initial_hash = static_cast<std::size_t>(14695981039346656037ULL & std::numeric_limits<std::size_t>::max());
            constexpr std::size_t fnv_prime = static_cast<std::size_t>(1099511628211ULL & std::numeric_limits<std::size_t>::max());

            std::size_t hash = initial_hash;

            for (const char c : value)
            {
                hash = (hash ^ static_cast<std::size_t>(c)) * fnv_prime;
            }

            return hash;
        };

        constexpr std::source_location location = std::source_location::current();
        constexpr std::size_t hash = hash_string(location.function_name());
        return static_cast<std::size_t>(hash);
    }
}