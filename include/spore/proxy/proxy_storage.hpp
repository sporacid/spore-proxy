#pragma once

#include "spore/proxy/proxy_macros.hpp"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <memory>
#include <optional>
#include <utility>
#include <variant>

namespace spore
{
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

    template <typename counter_t>
    struct proxy_storage_shared
    {
        // TODO @sporacid fix shared storage being different from others with block dispatch.

        proxy_storage_shared()
            : _dispatch(nullptr),
              _ptr(nullptr)
        {
        }

        template <typename value_t, typename... args_t>
        explicit proxy_storage_shared(std::in_place_type_t<value_t>, args_t&&... args) SPORE_PROXY_THROW_SPEC
        {
            shared_block<value_t>* block = new shared_block<value_t> {.counter {1}, .value {std::forward<args_t>(args)...}};

            _dispatch = std::addressof(proxy_storage_dispatch::get<shared_block<value_t>>());
            _ptr = block;
        }

        proxy_storage_shared(const proxy_storage_shared& other) noexcept
        {
            _dispatch = other._dispatch;
            _ptr = other._ptr;

            if (_dispatch != nullptr)
            {
                ++counter();
            }
        }

        proxy_storage_shared& operator=(const proxy_storage_shared& other) noexcept
        {
            reset();

            *this = proxy_storage_shared {other};

            return *this;
        }

        proxy_storage_shared(proxy_storage_shared&& other) noexcept
        {
            _dispatch = other._dispatch;
            _ptr = other._ptr;

            other._dispatch = nullptr;
            other._ptr = nullptr;
        }

        proxy_storage_shared& operator=(proxy_storage_shared&& other) noexcept
        {
            std::swap(_dispatch, other._dispatch);
            std::swap(_ptr, other._ptr);

            return *this;
        }

        ~proxy_storage_shared() noexcept
        {
            reset();
        }

        [[nodiscard]] void* ptr() const noexcept
        {
            return _ptr != nullptr ? std::addressof(static_cast<shared_block<std::byte>*>(_ptr)->value) : nullptr;
        }

        [[nodiscard]] const proxy_storage_dispatch* dispatch() const noexcept
        {
            return _dispatch;
        }

        void reset() noexcept
        {
            if (_dispatch != nullptr)
            {
                if (--counter() == 0)
                {
                    SPORE_PROXY_ASSERT(_ptr != nullptr);

                    _dispatch->destroy(_ptr);
                    _dispatch->deallocate(_ptr);
                }

                _dispatch = nullptr;
                _ptr = nullptr;
            }
        }

        counter_t& counter() noexcept
        {
            SPORE_PROXY_ASSERT(_ptr != nullptr);
            return static_cast<shared_block<std::byte>*>(_ptr)->counter;
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
    };

    struct proxy_storage_unique
    {
        proxy_storage_unique()
            : _dispatch(nullptr),
              _ptr(nullptr)
        {
        }

        template <typename storage_t>
        constexpr explicit proxy_storage_unique(storage_t&& storage) noexcept
            requires(any_proxy_storage<std::decay_t<storage_t>>)
        {
            _dispatch = storage.dispatch();

            if (_dispatch != nullptr)
            {
                _ptr = _dispatch->allocate();

                if constexpr (std::is_rvalue_reference_v<storage_t>)
                {
                    if constexpr (std::is_move_constructible_v<std::decay_t<storage_t>>)
                    {
                        _dispatch->move(_ptr, storage.ptr());
                    }
                    else
                    {
                        static_assert(std::is_copy_constructible_v<std::decay_t<storage_t>>);

                        _dispatch->copy(_ptr, storage.ptr());
                    }
                }
                else
                {
                    static_assert(std::is_copy_constructible_v<std::decay_t<storage_t>>);

                    _dispatch->copy(_ptr, storage.ptr());
                }
            }
        }

        template <typename value_t, typename... args_t>
        explicit proxy_storage_unique(std::in_place_type_t<value_t>, args_t&&... args) SPORE_PROXY_THROW_SPEC
        {
            _dispatch = std::addressof(proxy_storage_dispatch::get<value_t>());
            _ptr = new value_t {std::forward<args_t>(args)...};
        }

        proxy_storage_unique(proxy_storage_unique&& other) noexcept
        {
            _dispatch = other._dispatch;
            _ptr = other._ptr;

            other._dispatch = nullptr;
            other._ptr = nullptr;
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

        [[nodiscard]] const proxy_storage_dispatch* dispatch() const noexcept
        {
            return _dispatch;
        }

        void reset() noexcept
        {
            if (_dispatch != nullptr)
            {
                SPORE_PROXY_ASSERT(_ptr != nullptr);

                _dispatch->destroy(_ptr);
                _dispatch->deallocate(_ptr);

                _dispatch = nullptr;
                _ptr = nullptr;
            }
        }

      private:
        const proxy_storage_dispatch* _dispatch;
        void* _ptr;
    };

    struct proxy_storage_value
    {
        proxy_storage_value()
            : _dispatch(nullptr),
              _ptr(nullptr)
        {
        }

        template <typename storage_t>
        constexpr explicit proxy_storage_value(storage_t&& storage) noexcept
            requires(any_proxy_storage<std::decay_t<storage_t>>)
        {
            _dispatch = storage.dispatch();

            if (_dispatch != nullptr)
            {
                _ptr = _dispatch->allocate();

                if constexpr (std::is_rvalue_reference_v<storage_t>)
                {
                    if constexpr (std::is_move_constructible_v<std::decay_t<storage_t>>)
                    {
                        _dispatch->move(_ptr, storage.ptr());
                    }
                    else
                    {
                        static_assert(std::is_copy_constructible_v<std::decay_t<storage_t>>);

                        _dispatch->copy(_ptr, storage.ptr());
                    }
                }
                else
                {
                    static_assert(std::is_copy_constructible_v<std::decay_t<storage_t>>);

                    _dispatch->copy(_ptr, storage.ptr());
                }
            }
        }

        template <typename value_t, typename... args_t>
        explicit proxy_storage_value(std::in_place_type_t<value_t>, args_t&&... args) SPORE_PROXY_THROW_SPEC
        {
            _dispatch = std::addressof(proxy_storage_dispatch::get<value_t>());
            _ptr = _dispatch->allocate();
            std::construct_at(reinterpret_cast<value_t*>(_ptr), std::forward<args_t>(args)...);
        }

        proxy_storage_value(proxy_storage_value&& other) noexcept
        {
            _dispatch = other._dispatch;

            if (_dispatch != nullptr)
            {
                _ptr = _dispatch->allocate();
                _dispatch->move(ptr(), other.ptr());
            }
            else
            {
                _ptr = nullptr;
            }
        }

        proxy_storage_value(const proxy_storage_value& other) SPORE_PROXY_THROW_SPEC
        {
            _dispatch = other._dispatch;

            if (_dispatch != nullptr)
            {
                _ptr = _dispatch->allocate();
                _dispatch->copy(ptr(), other.ptr());
            }
            else
            {
                _ptr = nullptr;
            }
        }

        ~proxy_storage_value() noexcept
        {
            reset();
        }

        proxy_storage_value& operator=(proxy_storage_value&& other) noexcept
        {
            reset();

            _dispatch = other._dispatch;

            if (_dispatch != nullptr)
            {
                _ptr = _dispatch->allocate();
                _dispatch->move(ptr(), other.ptr());
            }
            else
            {
                _ptr = nullptr;
            }

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
            else
            {
                _ptr = nullptr;
            }

            return *this;
        }

        [[nodiscard]] void* ptr() const noexcept
        {
            return _ptr;
        }

        [[nodiscard]] const proxy_storage_dispatch* dispatch() const noexcept
        {
            return _dispatch;
        }

        void reset() noexcept
        {
            if (_dispatch != nullptr)
            {
                SPORE_PROXY_ASSERT(_ptr != nullptr);

                _dispatch->destroy(_ptr);
                _dispatch->deallocate(_ptr);

                _dispatch = nullptr;
                _ptr = nullptr;
            }
        }

      private:
        const proxy_storage_dispatch* _dispatch;
        void* _ptr;
    };

    struct proxy_storage_non_owning
    {
        proxy_storage_non_owning()
            : _dispatch(nullptr),
              _ptr(nullptr)
        {
        }

        template <typename storage_t>
        constexpr explicit proxy_storage_non_owning(storage_t&& storage) noexcept
            requires(any_proxy_storage<std::decay_t<storage_t>>)
        {
            _dispatch = storage.dispatch();
            _ptr = storage.ptr();
        }

        template <typename value_t>
        constexpr explicit proxy_storage_non_owning(std::in_place_type_t<value_t>, value_t& value) noexcept
        {
            _dispatch = std::addressof(proxy_storage_dispatch::get<value_t>());
            _ptr = std::addressof(value);
        }

        template <typename value_t>
        constexpr explicit proxy_storage_non_owning(std::in_place_type_t<value_t>, value_t&& value) noexcept
        {
            _dispatch = std::addressof(proxy_storage_dispatch::get<value_t>());
            _ptr = std::addressof(value);
        }

        template <typename value_t>
        constexpr explicit proxy_storage_non_owning(std::in_place_type_t<value_t>, const value_t& value) noexcept
        {
            _dispatch = std::addressof(proxy_storage_dispatch::get<value_t>());
            _ptr = std::addressof(const_cast<value_t&>(value));
        }

        [[nodiscard]] constexpr void* ptr() const noexcept
        {
            return _ptr;
        }

        constexpr void reset() noexcept
        {
            _dispatch = nullptr;
            _ptr = nullptr;
        }

        [[nodiscard]] constexpr const proxy_storage_dispatch* dispatch() const noexcept
        {
            return _dispatch;
        }

      private:
        const proxy_storage_dispatch* _dispatch;
        void* _ptr;
    };

    template <std::size_t size_v, std::size_t align_v = alignof(void*)>
    struct proxy_storage_sbo
    {
        static_assert(size_v > 0);
        static_assert(align_v <= size_v);

        proxy_storage_sbo()
            : _dispatch(nullptr)
        {
            std::ranges::fill(_storage, 0);
        }

        template <typename value_t, typename... args_t>
        constexpr explicit proxy_storage_sbo(std::in_place_type_t<value_t>, args_t&&... args) SPORE_PROXY_THROW_SPEC
            requires(is_compatible<value_t>())
        {
            _dispatch = std::addressof(proxy_storage_dispatch::get<value_t>());
            std::construct_at(static_cast<value_t*>(ptr()), std::forward<args_t>(args)...);
        }

        constexpr proxy_storage_sbo(const proxy_storage_sbo& other) SPORE_PROXY_THROW_SPEC
        {
            _dispatch = other._dispatch;

            if (_dispatch != nullptr)
            {
                _dispatch->copy(ptr(), other.ptr());
            }
        }

        constexpr proxy_storage_sbo(proxy_storage_sbo&& other) noexcept
        {
            _dispatch = other._dispatch;

            if (_dispatch != nullptr)
            {
                _dispatch->move(ptr(), other.ptr());
            }
        }

        constexpr ~proxy_storage_sbo() noexcept
        {
            reset();
        }

        constexpr proxy_storage_sbo& operator=(const proxy_storage_sbo& other) SPORE_PROXY_THROW_SPEC
        {
            reset();

            _dispatch = other._dispatch;

            if (_dispatch != nullptr)
            {
                _dispatch->copy(ptr(), other.ptr());
            }

            return *this;
        }

        constexpr proxy_storage_sbo& operator=(proxy_storage_sbo&& other) SPORE_PROXY_THROW_SPEC
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
            return _dispatch != nullptr ? std::addressof(_storage[0]) : nullptr;
        }

        [[nodiscard]] constexpr const proxy_storage_dispatch* dispatch() const noexcept
        {
            return _dispatch;
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
        const proxy_storage_dispatch* _dispatch;
        alignas(align_v) mutable std::byte _storage[size_v];

        template <typename value_t>
        static consteval bool is_compatible()
        {
            constexpr bool is_size_compatible = sizeof(value_t) <= size_v;
            constexpr bool is_alignment_compatible = align_v % alignof(value_t) == 0;
            return is_size_compatible and is_alignment_compatible;
        }
    };

    template <typename value_t>
    struct proxy_storage_inline
    {
        proxy_storage_inline() = default;

        template <typename... args_t>
        constexpr explicit proxy_storage_inline(std::in_place_type_t<value_t>, args_t&&... args) SPORE_PROXY_THROW_SPEC
        {
            _storage.emplace(std::forward<args_t>(args)...);
        }

        [[nodiscard]] constexpr void* ptr() const
        {
            return _storage.has_value() ? std::addressof(_storage.value()) : nullptr;
        }

        [[nodiscard]] constexpr const proxy_storage_dispatch* dispatch() const noexcept
        {
            return std::addressof(proxy_storage_dispatch::get<value_t>());
        }

        void reset() noexcept
        {
            _storage = std::nullopt;
        }

      private:
        mutable std::optional<value_t> _storage;
    };

    template <any_proxy_storage... storages_t>
    struct proxy_storage_chain
    {
        proxy_storage_chain() = default;

        template <typename value_t, typename... args_t>
        explicit proxy_storage_chain(std::in_place_type_t<value_t> type, args_t&&... args) SPORE_PROXY_THROW_SPEC
        {
            std::ignore = (... or try_construct<storages_t>(type, std::forward<args_t>(args)...));
        }

        [[nodiscard]] void* ptr() const noexcept
        {
            constexpr auto visitor = [](const auto& storage) { return storage.ptr(); };
            return _storage.has_value() ? std::visit(visitor, _storage.value()) : nullptr;
        }

        [[nodiscard]] constexpr const proxy_storage_dispatch* dispatch() const noexcept
        {
            constexpr auto visitor = [](const auto& storage) { return storage.dispatch(); };
            return _storage.has_value() ? std::visit(visitor, _storage.value()) : nullptr;
        }

        void reset() noexcept
        {
            _storage = std::nullopt;
        }

      private:
        std::optional<std::variant<storages_t...>> _storage;

        template <typename storage_t, typename value_t, typename... args_t>
        [[nodiscard]] constexpr bool try_construct(std::in_place_type_t<value_t> type, args_t&&... args)
        {
            if constexpr (std::is_constructible_v<storage_t, std::in_place_type_t<value_t>, args_t&&...>)
            {
                _storage.emplace(std::in_place_type<storage_t>, type, std::forward<args_t>(args)...);
                return true;
            }
            else
            {
                return false;
            }
        }
    };
}