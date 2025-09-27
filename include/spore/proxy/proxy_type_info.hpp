#pragma once

#include "spore/proxy/proxy_macros.hpp"

#include <cstddef>

namespace spore
{
    struct proxy_type_info
    {
        std::size_t size;
        std::size_t alignment;
        void* (*allocate)();
        void (*deallocate)(void*) noexcept;
        void (*destroy)(void*) noexcept;
        void (*move)(void*, void*);
        void (*copy)(void*, const void*);
    };

    namespace proxies::detail
    {
        template <typename value_t>
        const proxy_type_info& type_info()
        {
            // clang-format off
            static const proxy_type_info type_info {
                .size = sizeof(value_t),
                .alignment = alignof(value_t),
                .allocate = []() SPORE_PROXY_THROW_SPEC {
                    auto* value = std::allocator<value_t>().allocate(1);
                    return static_cast<void*>(value);
                },
                .deallocate = [](void* ptr) noexcept {
                    auto* value = static_cast<value_t*>(ptr);
                    std::allocator<value_t>().deallocate(value, 1);
                },
                .destroy = [](void* ptr) noexcept {
                    if constexpr (std::is_destructible_v<value_t>)
                    {
                        auto* value = static_cast<value_t*>(ptr);
                        std::destroy_at(value);
                    }
                },
                .move = [](void* ptr, void* other_ptr) SPORE_PROXY_THROW_SPEC {
                    if constexpr (std::is_trivially_move_constructible_v<value_t>)
                    {
                        std::memcpy(ptr, other_ptr, sizeof(value_t));
                    }
                    else if constexpr (std::is_move_constructible_v<value_t>)
                    {
                        auto* value = static_cast<value_t*>(ptr);
                        auto* other_value = static_cast<value_t*>(other_ptr);
                        std::construct_at(value, std::move(*other_value));
                    }
                    else
                    {
                        SPORE_PROXY_THROW("not movable");
                    }
                },
                .copy = [](void* ptr, const void* other_ptr) SPORE_PROXY_THROW_SPEC {
                    if constexpr (std::is_trivially_copy_constructible_v<value_t>)
                    {
                        std::memcpy(ptr, other_ptr, sizeof(value_t));
                    }
                    else if constexpr (std::is_copy_constructible_v<value_t>)
                    {
                        auto* value = static_cast<value_t*>(ptr);
                        const auto* other_value = static_cast<const value_t*>(other_ptr);
                        std::construct_at(value, *other_value);
                    }
                    else
                    {
                        throw std::runtime_error("not copyable");
                    }
                },
            };
            // clang-format on

            return type_info;
        }
    }
}