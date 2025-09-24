#pragma once

#include "spore/proxy/proxy_macros.hpp"

#include <concepts>
#include <cstddef>
#include <memory>
#include <utility>
#include <variant>

namespace spore
{
    // clang-format off
    template <typename storage_t>
    concept any_proxy_storage = requires(const storage_t& storage)
    {
        std::is_default_constructible_v<storage_t>;
        { storage_t(std::in_place_type<std::byte>, static_cast<std::byte>(0)) };
        { storage.ptr() } -> std::same_as<void*>;
    };
    // clang-format on

    struct proxy_storage_dispatch
    {
        void* (*allocate)();
        void (*deallocate)(void*) noexcept;
        void (*destroy)(void*) noexcept;
        void (*move)(void*, void*);
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

            return dispatch;
        }
    };

    struct proxy_storage_invalid
    {
        proxy_storage_invalid() = default;

        template <typename value_t, typename... args_t>
        explicit proxy_storage_invalid(std::in_place_type_t<value_t>, args_t&&...) SPORE_PROXY_THROW_SPEC
        {
            SPORE_PROXY_THROW("invalid storage");
        }

        [[noreturn]] static void* ptr() SPORE_PROXY_THROW_SPEC
        {
            SPORE_PROXY_THROW("invalid storage");
        }
    };

    struct proxy_storage_shared
    {
        proxy_storage_shared() = default;

        template <typename value_t, typename... args_t>
        explicit proxy_storage_shared(std::in_place_type_t<value_t>, args_t&&... args) SPORE_PROXY_THROW_SPEC
        {
            _ptr = std::make_shared<value_t>(std::forward<args_t>(args)...);
        }

        [[nodiscard]] void* ptr() const noexcept
        {
            return _ptr.get();
        }

      private:
        std::shared_ptr<void> _ptr;
    };

    struct proxy_storage_unique
    {
        proxy_storage_unique() = default;

        template <typename value_t, typename... args_t>
        explicit proxy_storage_unique(std::in_place_type_t<value_t>, args_t&&... args) SPORE_PROXY_THROW_SPEC
        {
            const auto& dispatch = proxy_storage_dispatch::get<value_t>();
            _ptr = std::unique_ptr<void, void (*)(void*)> {
                new value_t {std::forward<args_t>(args)...},
                dispatch.destroy,
            };
        }

        [[nodiscard]] void* ptr() const noexcept
        {
            return _ptr.get();
        }

      private:
        std::unique_ptr<void, void (*)(void*)> _ptr {nullptr, nullptr};
    };

    struct proxy_storage_value
    {
        proxy_storage_value() = default;

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

    template <std::size_t size_v, std::size_t align_v = alignof(void*)>
    struct proxy_storage_inline
    {
        static_assert(size_v > 0);
        static_assert(align_v <= size_v);

        template <typename value_t>
        static consteval bool is_compatible()
        {
            constexpr bool is_size_compatible = sizeof(value_t) <= size_v;
            constexpr bool is_alignment_compatible = align_v % alignof(value_t) == 0;
            return is_size_compatible and is_alignment_compatible;
        }

        constexpr proxy_storage_inline() = default;

        template <typename value_t, typename... args_t>
        constexpr explicit proxy_storage_inline(std::in_place_type_t<value_t>, args_t&&... args) requires(is_compatible<value_t>()) SPORE_PROXY_THROW_SPEC
        {
            _dispatch = std::addressof(proxy_storage_dispatch::get<value_t>());

            auto* ptr = reinterpret_cast<value_t*>(std::addressof(_storage));
            std::construct_at(ptr, std::forward<args_t>(args)...);
        }

        constexpr proxy_storage_inline(const proxy_storage_inline& other) SPORE_PROXY_THROW_SPEC
        {
            _dispatch = other._dispatch;

            if (_dispatch != nullptr)
            {
                _dispatch->copy(ptr(), other.ptr());
            }
        }

        constexpr proxy_storage_inline(proxy_storage_inline&& other) noexcept
        {
            _dispatch = other._dispatch;

            if (_dispatch != nullptr)
            {
                _dispatch->move(ptr(), other.ptr());
            }
        }

        constexpr ~proxy_storage_inline() noexcept
        {
            if (_dispatch != nullptr)
            {
                _dispatch->destroy(ptr());
            }
        }

        constexpr proxy_storage_inline& operator=(const proxy_storage_inline& other) SPORE_PROXY_THROW_SPEC
        {
            reset();

            _dispatch = other._dispatch;

            if (_dispatch != nullptr)
            {
                _dispatch->copy(ptr(), other.ptr());
            }

            return *this;
        }

        constexpr proxy_storage_inline& operator=(proxy_storage_inline&& other) SPORE_PROXY_THROW_SPEC
        {
            reset();

            _dispatch = other._dispatch;

            if (_dispatch != nullptr)
            {
                _dispatch->move(ptr(), other.ptr());
            }

            return *this;
        }

        [[nodiscard]] constexpr void* ptr() const noexcept
        {
            return const_cast<std::byte*>(std::addressof(_storage[0]));
        }

        void reset() noexcept
        {
            if (_dispatch != nullptr)
            {
                _dispatch->destroy(ptr());
                _dispatch = nullptr;
            }
        }

      private:
        alignas(align_v) std::byte _storage[size_v];
        const proxy_storage_dispatch* _dispatch = nullptr;
    };

    template <any_proxy_storage fallback_storage_t, std::size_t size_v = sizeof(fallback_storage_t), std::size_t align_v = alignof(void*)>
    struct proxy_storage_inline_or
    {
        proxy_storage_inline_or() = default;

        template <typename value_t, typename... args_t>
        explicit proxy_storage_inline_or(std::in_place_type_t<value_t> type, args_t&&... args) SPORE_PROXY_THROW_SPEC
        {
            if constexpr (inline_storage_t::template is_compatible<value_t>())
            {
                _storage.template emplace<inline_storage_t>(type, std::forward<args_t>(args)...);
            }
            else
            {
                _storage.template emplace<fallback_storage_t>(type, std::forward<args_t>(args)...);
            }
        }

        [[nodiscard]] void* ptr() const noexcept
        {
            constexpr auto visitor = [](const auto& storage) { return storage.ptr(); };
            return std::visit(visitor, _storage);
        }

      private:
        using inline_storage_t = proxy_storage_inline<size_v, align_v>;
        std::variant<proxy_storage_invalid, inline_storage_t, fallback_storage_t> _storage;
    };
}