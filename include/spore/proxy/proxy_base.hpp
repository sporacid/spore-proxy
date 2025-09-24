#pragma once

#include <cstddef>

namespace spore
{
    struct proxy_base
    {
        explicit proxy_base(const std::uint32_t type_index)
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