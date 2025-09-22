#pragma once

#include "spore/proxy/detail/proxy_hash.hpp"

#include <source_location>

namespace spore::proxies::detail
{
    template <typename value_t>
    consteval std::size_t type_id()
    {
        constexpr std::source_location location = std::source_location::current();
        constexpr std::size_t hash = proxies::detail::hash_string(location.function_name());
        return static_cast<std::size_t>(hash);
    }
}