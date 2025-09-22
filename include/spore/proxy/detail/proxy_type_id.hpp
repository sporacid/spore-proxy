#pragma once

#include <cstdint>
#include <limits>
#include <source_location>
#include <string_view>

#include <ranges>

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

    consteval std::size_t hash_source_location(std::source_location location)
    {
        constexpr auto hash_bytes = []<typename value_t>(std::span<const value_t> bytes) {
            constexpr std::size_t initial_hash = static_cast<std::size_t>(14695981039346656037ULL & std::numeric_limits<std::size_t>::max());
            constexpr std::size_t fnv_prime = static_cast<std::size_t>(1099511628211ULL & std::numeric_limits<std::size_t>::max());

            std::size_t hash = initial_hash;

            for (const value_t c : bytes)
            {
                hash = (hash ^ static_cast<std::size_t>(c)) * fnv_prime;
            }

            return hash;
        };

        constexpr auto hash_combine = [](std::size_t hash1, std::size_t hash2) {
            return hash1 ^ (hash1 + 0x9e3779b9 + (hash2 << 6) + (hash2 >> 2));
        };

        const uint_least32_t line = location.line();
        const uint_least32_t column = location.column();
        const std::string_view file_name = location.file_name();
        const std::string_view function_name = location.function_name();

        return hash_combine(
            hash_bytes(std::span {std::addressof(line), sizeof(uint_least32_t)}),
            hash_combine(
                hash_bytes(std::span {std::addressof(column), sizeof(uint_least32_t)}),
                hash_combine(
                    hash_bytes(std::span {file_name}),
                    hash_bytes(std::span {function_name}))));
    }
}