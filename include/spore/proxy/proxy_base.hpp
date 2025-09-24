#pragma once

#include <cstdint>
#include <limits>

namespace spore
{
    struct proxy_base
    {
        proxy_base() noexcept
            : _ptr(nullptr),
              _type_index(std::numeric_limits<std::uint32_t>::max())
        {
        }

        explicit proxy_base(const std::uint32_t type_index) noexcept
            : _ptr(nullptr),
              _type_index(type_index)
        {
        }

        [[nodiscard]] void* ptr() noexcept
        {
            return _ptr;
        }

        [[nodiscard]] const void* ptr() const noexcept
        {
            return _ptr;
        }

        [[nodiscard]] std::uint32_t type_index() const noexcept
        {
            return _type_index;
        }

      protected:
        void* _ptr;
        std::uint32_t _type_index;
    };
}