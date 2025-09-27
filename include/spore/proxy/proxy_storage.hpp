#pragma once

#include "spore/proxy/proxy_macros.hpp"
#include "spore/proxy/proxy_type_info.hpp"

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <optional>
#include <utility>
#include <variant>

namespace spore
{
    namespace proxies::detail
    {
        struct proxy_allocation_base
        {
            proxy_allocation_base() = default;

            void reset() noexcept
            {
                if (_type_info != nullptr)
                {
                    destroy();
                }
            }

            [[nodiscard]] const proxy_type_info* type_info() const noexcept
            {
                return _type_info;
            }

            [[nodiscard]] void* ptr() const noexcept
            {
                return _ptr;
            }

          protected:
            template <typename value_t, typename... args_t>
            void in_place_construct(args_t&&... args) SPORE_PROXY_THROW_SPEC
            {
                SPORE_PROXY_ASSERT(_type_info == nullptr);
                SPORE_PROXY_ASSERT(_ptr == nullptr);

                _type_info = std::addressof(proxies::detail::type_info<value_t>());
                _ptr = new value_t {std::forward<args_t>(args)...};
            }

            template <typename storage_t>
            void storage_construct(storage_t&& storage) SPORE_PROXY_THROW_SPEC
                requires(any_proxy_storage<std::decay_t<storage_t>>)
            {
                if (const proxy_type_info* type_info = storage.type_info())
                {
                    if constexpr (std::is_rvalue_reference_v<storage_t&&> and std::is_move_constructible_v<std::decay_t<storage_t>>)
                    {
                        move_construct(*type_info, storage.ptr());
                    }
                    else
                    {
                        static_assert(std::is_copy_constructible_v<std::decay_t<storage_t>>);

                        copy_construct(*type_info, storage.ptr());
                    }
                }
            }

            void move_construct(const proxy_type_info& type_info, void* ptr) SPORE_PROXY_THROW_SPEC
            {
                SPORE_PROXY_ASSERT(_type_info == nullptr);
                SPORE_PROXY_ASSERT(_ptr == nullptr);

                _type_info = std::addressof(type_info);
                _ptr = type_info.allocate();

                type_info.move(_ptr, ptr);
            }

            void copy_construct(const proxy_type_info& type_info, const void* ptr) SPORE_PROXY_THROW_SPEC
            {
                SPORE_PROXY_ASSERT(_type_info == nullptr);
                SPORE_PROXY_ASSERT(_ptr == nullptr);

                _type_info = std::addressof(type_info);
                _ptr = type_info.allocate();

                type_info.copy(_ptr, ptr);
            }

            void destroy() noexcept
            {
                SPORE_PROXY_ASSERT(_type_info != nullptr);
                SPORE_PROXY_ASSERT(_ptr != nullptr);

                _type_info->destroy(_ptr);
                _type_info->deallocate(_ptr);

                _type_info = nullptr;
                _ptr = nullptr;
            }

            const proxy_type_info* _type_info = nullptr;
            void* _ptr = nullptr;
        };
    }

    template <typename counter_t>
    struct proxy_storage_shared
    {
        proxy_storage_shared() = default;

        template <typename value_t, typename... args_t>
        explicit proxy_storage_shared(std::in_place_type_t<value_t>, args_t&&... args) SPORE_PROXY_THROW_SPEC
        {
            shared_block<value_t>* block = new shared_block<value_t> {std::forward<args_t>(args)...};

            _type_info = std::addressof(proxies::detail::type_info<value_t>());
            _block = block;
            _ptr = std::addressof(block->value);
        }

        proxy_storage_shared(const proxy_storage_shared& other) noexcept
        {
            _type_info = other._type_info;
            _block = other._block;
            _ptr = other._ptr;

            if (_type_info != nullptr)
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
            _type_info = other._type_info;
            _block = other._block;
            _ptr = other._ptr;

            other._type_info = nullptr;
            other._block = nullptr;
            other._ptr = nullptr;
        }

        proxy_storage_shared& operator=(proxy_storage_shared&& other) noexcept
        {
            std::swap(_type_info, other._type_info);
            std::swap(_block, other._block);
            std::swap(_ptr, other._ptr);

            return *this;
        }

        ~proxy_storage_shared() noexcept
        {
            reset();
        }

        void reset() noexcept
        {
            if (_type_info != nullptr)
            {
                if (--counter() == 0)
                {
                    SPORE_PROXY_ASSERT(_block != nullptr);

                    delete _block;
                }

                _type_info = nullptr;
                _block = nullptr;
                _ptr = nullptr;
            }
        }

        [[nodiscard]] const proxy_type_info* type_info() const noexcept
        {
            return _type_info;
        }

        [[nodiscard]] void* ptr() const noexcept
        {
            return _ptr;
        }

        [[nodiscard]] counter_t& counter() noexcept
        {
            SPORE_PROXY_ASSERT(_block != nullptr);
            return _block->counter;
        }

      private:
        struct shared_block_base
        {
            counter_t counter;
            virtual ~shared_block_base() = default;
        };

        template <typename value_t>
        struct shared_block : shared_block_base
        {
            value_t value;

            template <typename... args_t>
            explicit shared_block(args_t&&... args)
                : shared_block_base(), value {std::forward<args_t>(args)...}
            {
                shared_block_base::counter = 1;
            }
        };

        const proxy_type_info* _type_info = nullptr;
        shared_block_base* _block = nullptr;
        void* _ptr = nullptr;
    };

    struct proxy_storage_unique : proxies::detail::proxy_allocation_base
    {
        proxy_storage_unique() = default;

        template <typename value_t, typename... args_t>
        explicit proxy_storage_unique(std::in_place_type_t<value_t>, args_t&&... args) SPORE_PROXY_THROW_SPEC
        {
            in_place_construct<value_t>(std::forward<args_t>(args)...);
        }

        proxy_storage_unique(proxy_storage_unique&& other) noexcept
        {
            _type_info = other._type_info;
            _ptr = other._ptr;

            other._type_info = nullptr;
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
            std::swap(_type_info, other._type_info);
            std::swap(_ptr, other._ptr);

            return *this;
        }
    };

    struct proxy_storage_value : proxies::detail::proxy_allocation_base
    {
        proxy_storage_value() = default;

        template <typename storage_t>
        constexpr explicit proxy_storage_value(storage_t&& storage) noexcept
            requires(any_proxy_storage<std::decay_t<storage_t>>)
        {
            storage_construct(std::forward<storage_t>(storage));
        }

        template <typename value_t, typename... args_t>
        explicit proxy_storage_value(std::in_place_type_t<value_t>, args_t&&... args) SPORE_PROXY_THROW_SPEC
        {
            in_place_construct<value_t>(std::forward<args_t>(args)...);
        }

        proxy_storage_value(proxy_storage_value&& other) noexcept
        {
            if (const proxy_type_info* type_info = other.type_info())
            {
                move_construct(*type_info, other.ptr());
            }
        }

        proxy_storage_value(const proxy_storage_value& other) SPORE_PROXY_THROW_SPEC
        {
            if (const proxy_type_info* type_info = other.type_info())
            {
                copy_construct(*type_info, other.ptr());
            }
        }

        ~proxy_storage_value() noexcept
        {
            reset();
        }

        proxy_storage_value& operator=(proxy_storage_value&& other) noexcept
        {
            reset();

            if (const proxy_type_info* type_info = other.type_info())
            {
                move_construct(*type_info, other.ptr());
            }

            return *this;
        }

        proxy_storage_value& operator=(const proxy_storage_value& other) SPORE_PROXY_THROW_SPEC
        {
            reset();

            if (const proxy_type_info* type_info = other.type_info())
            {
                copy_construct(*type_info, other.ptr());
            }

            return *this;
        }

        [[nodiscard]] static constexpr bool is_constructible(const proxy_type_info&)
        {
            return true;
        }
    };

    struct proxy_storage_non_owning
    {
        proxy_storage_non_owning() = default;

        template <typename value_t>
        constexpr explicit proxy_storage_non_owning(std::in_place_type_t<value_t>, SPORE_PROXY_LIFETIME_BOUND value_t& value) noexcept
        {
            _type_info = std::addressof(proxies::detail::type_info<value_t>());
            _ptr = std::addressof(value);
        }

        template <typename value_t>
        constexpr explicit proxy_storage_non_owning(std::in_place_type_t<value_t>, SPORE_PROXY_LIFETIME_BOUND value_t&& value) noexcept
        {
            _type_info = std::addressof(proxies::detail::type_info<value_t>());
            _ptr = std::addressof(value);
        }

        template <typename value_t>
        constexpr explicit proxy_storage_non_owning(std::in_place_type_t<value_t>, SPORE_PROXY_LIFETIME_BOUND const value_t& value) noexcept
        {
            _type_info = std::addressof(proxies::detail::type_info<value_t>());
            _ptr = std::addressof(const_cast<value_t&>(value));
        }

        template <typename storage_t>
        constexpr explicit proxy_storage_non_owning(SPORE_PROXY_LIFETIME_BOUND storage_t&& storage) noexcept
            requires(any_proxy_storage<std::decay_t<storage_t>>)
        {
            _type_info = storage.type_info();
            _ptr = storage.ptr();
        }

        constexpr void reset() noexcept
        {
            _type_info = nullptr;
            _ptr = nullptr;
        }

        [[nodiscard]] constexpr const proxy_type_info* type_info() const noexcept
        {
            return _type_info;
        }

        [[nodiscard]] constexpr void* ptr() const noexcept
        {
            return _ptr;
        }

        [[nodiscard]] static constexpr bool is_constructible(const proxy_type_info&)
        {
            return true;
        }

      private:
        const proxy_type_info* _type_info = nullptr;
        void* _ptr = nullptr;
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

        template <typename storage_t>
        constexpr explicit proxy_storage_inline(storage_t&& storage) noexcept
            requires(any_proxy_storage<std::decay_t<storage_t>>)
        {
            if (const proxy_type_info* type_info = storage.type_info())
            {
                SPORE_PROXY_ASSERT(proxy_storage_inline::is_constructible(*type_info));

                if constexpr (std::is_rvalue_reference_v<storage_t&&> and std::is_move_constructible_v<std::decay_t<storage_t>>)
                {
                    type_info->move(ptr(), storage.ptr());
                }
                else
                {
                    static_assert(std::is_copy_constructible_v<std::decay_t<storage_t>>);

                    type_info->copy(ptr(), storage.ptr());
                }
            }
        }

        void reset() noexcept
        {
            _storage = std::nullopt;
        }

        [[nodiscard]] constexpr const proxy_type_info* type_info() const noexcept
        {
            return std::addressof(proxies::detail::type_info<value_t>());
        }

        [[nodiscard]] constexpr void* ptr() const noexcept
        {
            return _storage.has_value() ? std::addressof(_storage.value()) : nullptr;
        }

        [[nodiscard]] static constexpr bool is_constructible(const proxy_type_info& type_info)
        {
            return std::addressof(type_info) == std::addressof(proxies::detail::type_info<value_t>());
        }

      private:
        mutable std::optional<value_t> _storage;
    };

    template <std::size_t size_v, std::size_t align_v = alignof(void*)>
    struct proxy_storage_sbo
    {
        static_assert(size_v > 0);
        static_assert(align_v > 0);

        template <typename value_t>
        static consteval bool is_constructible()
        {
            return sizeof(value_t) <= size_v and align_v % alignof(value_t) == 0;
        }

        proxy_storage_sbo() = default;

        template <typename value_t, typename... args_t>
        constexpr explicit proxy_storage_sbo(std::in_place_type_t<value_t>, args_t&&... args) SPORE_PROXY_THROW_SPEC
            requires(is_constructible<value_t>())
        {
            _type_info = std::addressof(proxies::detail::type_info<value_t>());

            std::construct_at(static_cast<value_t*>(ptr()), std::forward<args_t>(args)...);
        }

        template <typename storage_t>
        constexpr explicit proxy_storage_sbo(storage_t&& storage) noexcept
            requires(any_proxy_storage<std::decay_t<storage_t>>)
        {
            if (const proxy_type_info* type_info = storage.type_info())
            {
                SPORE_PROXY_ASSERT(proxy_storage_sbo::is_constructible(*type_info));

                _type_info = type_info;

                if constexpr (std::is_rvalue_reference_v<storage_t&&> and std::is_move_constructible_v<std::decay_t<storage_t>>)
                {
                    _type_info->move(ptr(), storage.ptr());
                }
                else
                {
                    static_assert(std::is_copy_constructible_v<std::decay_t<storage_t>>);

                    _type_info->copy(ptr(), storage.ptr());
                }
            }
        }

        constexpr proxy_storage_sbo(const proxy_storage_sbo& other) SPORE_PROXY_THROW_SPEC
        {
            _type_info = other._type_info;

            if (_type_info != nullptr)
            {
                _type_info->copy(ptr(), other.ptr());
            }
        }

        constexpr proxy_storage_sbo(proxy_storage_sbo&& other) noexcept
        {
            _type_info = other._type_info;

            if (_type_info != nullptr)
            {
                _type_info->move(ptr(), other.ptr());
            }
        }

        constexpr ~proxy_storage_sbo() noexcept
        {
            reset();
        }

        constexpr proxy_storage_sbo& operator=(const proxy_storage_sbo& other) SPORE_PROXY_THROW_SPEC
        {
            reset();

            _type_info = other._type_info;

            if (_type_info != nullptr)
            {
                _type_info->copy(ptr(), other.ptr());
            }

            return *this;
        }

        constexpr proxy_storage_sbo& operator=(proxy_storage_sbo&& other) SPORE_PROXY_THROW_SPEC
        {
            reset();

            _type_info = other._type_info;

            if (_type_info != nullptr)
            {
                _type_info->move(ptr(), other.ptr());
            }

            return *this;
        }

        void reset() noexcept
        {
            if (_type_info != nullptr)
            {
                _type_info->destroy(ptr());
                _type_info = nullptr;
            }
        }

        [[nodiscard]] constexpr const proxy_type_info* type_info() const noexcept
        {
            return _type_info;
        }

        [[nodiscard]] constexpr void* ptr() const noexcept
        {
            return _type_info != nullptr ? std::addressof(_storage[0]) : nullptr;
        }

        [[nodiscard]] static constexpr bool is_constructible(const proxy_type_info& type_info)
        {
            return type_info.size <= size_v and align_v % type_info.alignment == 0;
        }

      private:
        const proxy_type_info* _type_info = nullptr;
        alignas(align_v) mutable std::array<std::byte, size_v> _storage {};
    };

    template <any_proxy_storage... storages_t>
    struct proxy_storage_chain
    {
        proxy_storage_chain() = default;

        template <typename value_t, typename... args_t>
        constexpr explicit proxy_storage_chain(std::in_place_type_t<value_t> type, args_t&&... args) SPORE_PROXY_THROW_SPEC
        {
            std::ignore = (... or try_construct<storages_t>(type, std::forward<args_t>(args)...));
        }

        template <typename storage_t>
        constexpr explicit proxy_storage_chain(storage_t&& storage) noexcept
            requires(any_proxy_storage<std::decay_t<storage_t>>)
        {
            if (const proxy_type_info* type_info = storage.type_info())
            {
                std::ignore = (... or try_construct<storages_t>(*type_info, std::forward<storage_t>(storage)));
            }
        }

        constexpr void reset() noexcept
        {
            _storage = std::nullopt;
        }

        [[nodiscard]] constexpr const proxy_type_info* type_info() const noexcept
        {
            constexpr auto visitor = [](const auto& storage) { return storage.type_info(); };
            return _storage.has_value() ? std::visit(visitor, _storage.value()) : nullptr;
        }

        [[nodiscard]] constexpr void* ptr() const noexcept
        {
            constexpr auto visitor = [](const auto& storage) { return storage.ptr(); };
            return _storage.has_value() ? std::visit(visitor, _storage.value()) : nullptr;
        }

        [[nodiscard]] static constexpr bool is_constructible(const proxy_type_info& type_info)
        {
            return (... or storages_t::is_constructible_from(type_info));
        }

      private:
        std::optional<std::variant<storages_t...>> _storage;

        template <typename storage_t, typename forward_storage_t>
        [[nodiscard]] constexpr bool try_construct(const proxy_type_info& type_info, forward_storage_t&& storage)
        {
            constexpr bool is_constructible_callable = requires {
                { storage_t::is_constructible(type_info) } -> std::convertible_to<bool>;
            };

            if constexpr (is_constructible_callable)
            {
                if (storage_t::is_constructible(type_info))
                {
                    _storage.emplace(std::in_place_type<storage_t>, std::forward<forward_storage_t>(storage));

                    return true;
                }
            }

            return false;
        }

        template <typename storage_t, typename value_t, typename... args_t>
        [[nodiscard]] constexpr bool try_construct(std::in_place_type_t<value_t> type, args_t&&... args)
        {
            if constexpr (std::is_constructible_v<storage_t, std::in_place_type_t<value_t>, args_t&&...>)
            {
                _storage.emplace(std::in_place_type<storage_t>, type, std::forward<args_t>(args)...);

                return true;
            }

            return false;
        }
    };
}