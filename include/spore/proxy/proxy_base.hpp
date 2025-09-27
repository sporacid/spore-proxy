#pragma once

#include "spore/proxy/proxy_macros.hpp"

#include <cstdint>
#include <limits>
#include <optional>

namespace spore
{
    struct proxy_base
    {
        proxy_base() noexcept
            : _ptr(nullptr),
              _type_index(std::nullopt)
        {
        }

        explicit proxy_base(const std::uint32_t type_index) noexcept
            : _ptr(nullptr),
              _type_index(type_index)
        {
        }

        [[nodiscard]] SPORE_PROXY_FORCE_INLINE void* ptr() noexcept
        {
            return _ptr;
        }

        [[nodiscard]] SPORE_PROXY_FORCE_INLINE const void* ptr() const noexcept
        {
            return _ptr;
        }

        [[nodiscard]] SPORE_PROXY_FORCE_INLINE std::uint32_t type_index() const noexcept
        {
            return _type_index.value();
        }

      protected:
        void* _ptr;
        std::optional<std::uint32_t> _type_index;
    };
}