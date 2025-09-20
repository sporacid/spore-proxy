#pragma once

#include <cstddef>

namespace spore
{
    struct proxy_base
    {
        explicit proxy_base(const std::size_t type_id)
            : _ptr(nullptr),
              _type_id(type_id)
        {
        }

        [[nodiscard]] void* ptr() const noexcept
        {
            return _ptr;
        }

        [[nodiscard]] std::size_t type_id() const noexcept
        {
            return _type_id;
        }

      protected:
        void* _ptr;
        std::size_t _type_id;
    };
}