#pragma once

#include "spore/proxy/proxy_macros.hpp"

#include <concepts>
#include <cstddef>
#include <memory>
#include <optional>
#include <utility>
#include <variant>

namespace spore
{
    // clang-format off
    template <typename storage_t>
    concept any_proxy_storage = requires(const storage_t& storage)
    {
        { storage.ptr() } -> std::same_as<void*>;
    };
    // clang-format on

    struct proxy_storage_dispatch
    {
        void* (*allocate)();
        void (*deallocate)(void*) noexcept;
        void (*destroy)(void*) noexcept;
        void (*copy)(void*, const void*);

        template <typename value_t>
        static const proxy_storage_dispatch& get()
        {
            // clang-format off
            static proxy_storage_dispatch dispatch {
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

            return dispatch;
        }
    };

    template <typename counter_t>
    struct proxy_storage_shared
    {
        template <typename value_t, typename... args_t>
        explicit proxy_storage_shared(std::in_place_type_t<value_t>, args_t&&... args) SPORE_PROXY_THROW_SPEC
        {
            _dispatch = std::addressof(proxy_storage_dispatch::get<shared_block<value_t>>());
            _ptr = new shared_block<value_t> {.counter {}, .value {std::forward<args_t>(args)...}};
        }

        proxy_storage_shared(proxy_storage_shared&& other) noexcept
        {
            std::swap(_dispatch, other._dispatch);
            std::swap(_ptr, other._ptr);
        }

        proxy_storage_shared(const proxy_storage_shared& other) noexcept
        {
            _dispatch = other._dispatch;
            _ptr = other._ptr;

            SPORE_PROXY_ASSERT(_dispatch != nullptr);
            SPORE_PROXY_ASSERT(_ptr != nullptr);

            ++counter();
        }

        proxy_storage_shared& operator=(proxy_storage_shared&& other) noexcept
        {
            std::swap(_dispatch, other._dispatch);
            std::swap(_ptr, other._ptr);

            return *this;
        }

        proxy_storage_shared& operator=(const proxy_storage_shared& other) noexcept
        {
            reset();

            _dispatch = other._dispatch;
            _ptr = other._ptr;

            SPORE_PROXY_ASSERT(_dispatch != nullptr);
            SPORE_PROXY_ASSERT(_ptr != nullptr);

            ++counter();

            return *this;
        }

        ~proxy_storage_shared() noexcept
        {
            reset();
        }

        [[nodiscard]] void* ptr() const noexcept
        {
            SPORE_PROXY_ASSERT(_ptr != nullptr);
            return std::addressof(static_cast<shared_block<std::byte>*>(_ptr)->value);
        }

        void reset() noexcept
        {
            SPORE_PROXY_ASSERT(_dispatch != nullptr);
            SPORE_PROXY_ASSERT(_ptr != nullptr);

            if (counter()-- == 0)
            {
                _dispatch->destroy(_ptr);
                _dispatch->deallocate(_ptr);
            }

            _dispatch = nullptr;
            _ptr = nullptr;
        }

      private:
        template <typename value_t>
        struct shared_block
        {
            counter_t counter;
            value_t value;
        };

        const proxy_storage_dispatch* _dispatch;
        void* _ptr;

        counter_t& counter() noexcept
        {
            SPORE_PROXY_ASSERT(_ptr != nullptr);
            return static_cast<shared_block<std::byte>*>(_ptr)->counter;
        }
    };

    struct proxy_storage_unique
    {
        template <typename value_t, typename... args_t>
        explicit proxy_storage_unique(std::in_place_type_t<value_t>, args_t&&... args) SPORE_PROXY_THROW_SPEC
        {
            _dispatch = std::addressof(proxy_storage_dispatch::get<value_t>());
            _ptr = new value_t {std::forward<args_t>(args)...};
        }

        proxy_storage_unique(proxy_storage_unique&& other) noexcept
        {
            std::swap(_dispatch, other._dispatch);
            std::swap(_ptr, other._ptr);
        }

        proxy_storage_unique(const proxy_storage_unique& other) = delete;
        proxy_storage_unique& operator=(const proxy_storage_unique& other) = delete;

        ~proxy_storage_unique() noexcept
        {
            reset();
        }

        proxy_storage_unique& operator=(proxy_storage_unique&& other) noexcept
        {
            std::swap(_dispatch, other._dispatch);
            std::swap(_ptr, other._ptr);

            return *this;
        }

        [[nodiscard]] void* ptr() const noexcept
        {
            return _ptr;
        }

        void reset() noexcept
        {
            if (_dispatch != nullptr and _ptr != nullptr)
            {
                _dispatch->destroy(_ptr);
                _dispatch->deallocate(_ptr);
                _dispatch = nullptr;
                _ptr = nullptr;
            }
        }

      private:
        const proxy_storage_dispatch* _dispatch = nullptr;
        void* _ptr = nullptr;
    };

    struct proxy_storage_value
    {
        template <typename value_t, typename... args_t>
        explicit proxy_storage_value(std::in_place_type_t<value_t>, args_t&&... args) SPORE_PROXY_THROW_SPEC
        {
            _dispatch = std::addressof(proxy_storage_dispatch::get<value_t>());
            _ptr = _dispatch->allocate();
            std::construct_at(reinterpret_cast<value_t*>(_ptr), std::forward<args_t>(args)...);
        }

        proxy_storage_value(proxy_storage_value&& other) noexcept
        {
            std::swap(_dispatch, other._dispatch);
            std::swap(_ptr, other._ptr);
        }

        proxy_storage_value(const proxy_storage_value& other) SPORE_PROXY_THROW_SPEC
        {
            _dispatch = other._dispatch;

            if (_dispatch != nullptr)
            {
                _ptr = _dispatch->allocate();
                _dispatch->copy(ptr(), other.ptr());
            }
        }

        ~proxy_storage_value() noexcept
        {
            reset();
        }

        proxy_storage_value& operator=(proxy_storage_value&& other) noexcept
        {
            std::swap(_dispatch, other._dispatch);
            std::swap(_ptr, other._ptr);

            return *this;
        }

        proxy_storage_value& operator=(const proxy_storage_value& other) SPORE_PROXY_THROW_SPEC
        {
            reset();

            _dispatch = other._dispatch;

            if (_dispatch != nullptr)
            {
                _ptr = _dispatch->allocate();
                _dispatch->copy(ptr(), other.ptr());
            }

            return *this;
        }

        [[nodiscard]] void* ptr() const noexcept
        {
            return _ptr;
        }

        void reset() noexcept
        {
            if (_dispatch != nullptr and _ptr != nullptr)
            {
                _dispatch->destroy(_ptr);
                _dispatch->deallocate(_ptr);
                _dispatch = nullptr;
                _ptr = nullptr;
            }
        }

      private:
        const proxy_storage_dispatch* _dispatch = nullptr;
        void* _ptr = nullptr;
    };

    template <typename value_t>
    struct proxy_storage_inline
    {
        template <typename... args_t>
        constexpr explicit proxy_storage_inline(std::in_place_type_t<value_t>, args_t&&... args) SPORE_PROXY_THROW_SPEC
        {
            value.emplace(std::forward<args_t>(args)...);
        }

        [[nodiscard]] constexpr void* ptr() const
        {
            return std::addressof(value.value());
        }

      private:
        mutable std::optional<value_t> value;
    };
}