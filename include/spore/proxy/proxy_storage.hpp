#pragma once

#include "spore/proxy/proxy_macros.hpp"
#include "spore/proxy/proxy_type_info.hpp"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <memory>
#include <optional>
#include <utility>
#include <variant>

namespace spore
{
#if 1
    template <typename counter_t>
    struct proxy_storage_shared
    {
        // TODO @sporacid fix shared storage being different from others with block type_info.

        proxy_storage_shared()
            : _type_info(nullptr),
              _block(nullptr)
        {
        }

        template <typename value_t, typename... args_t>
        explicit proxy_storage_shared(std::in_place_type_t<value_t>, args_t&&... args) SPORE_PROXY_THROW_SPEC
        {
            shared_block<value_t>* block = new shared_block<value_t> {std::forward<args_t>(args)...};

            _type_info = std::addressof(proxies::detail::type_info<value_t>());
            _block = block;
        }

        proxy_storage_shared(const proxy_storage_shared& other) noexcept
        {
            _type_info = other._type_info;
            _block = other._block;

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

            other._type_info = nullptr;
            other._block = nullptr;
        }

        proxy_storage_shared& operator=(proxy_storage_shared&& other) noexcept
        {
            std::swap(_type_info, other._type_info);
            std::swap(_block, other._block);

            return *this;
        }

        ~proxy_storage_shared() noexcept
        {
            reset();
        }

        [[nodiscard]] void* ptr() const noexcept
        {
            return _block != nullptr ? _block->ptr() : nullptr;
        }

        [[nodiscard]] const proxy_type_info* type_info() const noexcept
        {
            return _type_info;
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
            }
        }

        counter_t& counter() noexcept
        {
            SPORE_PROXY_ASSERT(_block != nullptr);
            return _block->counter;
        }

      private:
        struct shared_block_base
        {
            counter_t counter;
            shared_block_base() : counter(1) {}
            virtual void* ptr() noexcept = 0;
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
            }

            void* ptr() noexcept override
            {
                return std::addressof(value);
            }
        };

        const proxy_type_info* _type_info;
        shared_block_base* _block;
    };
#elif 0
    template <typename counter_t>
    struct proxy_storage_shared
    {
        // TODO @sporacid fix shared storage being different from others with block type_info.

        proxy_storage_shared()
            : _type_info(nullptr),
              _ptr(nullptr)
        {
        }

        template <typename value_t, typename... args_t>
        explicit proxy_storage_shared(std::in_place_type_t<value_t>, args_t&&... args) SPORE_PROXY_THROW_SPEC
        {
            shared_block<value_t>* block = new shared_block<value_t> {.counter {1}, .value {std::forward<args_t>(args)...}};

            _type_info = std::addressof(proxy_storage_type_info::get<shared_block<value_t>>());
            _ptr = block;
        }

        proxy_storage_shared(const proxy_storage_shared& other) noexcept
        {
            _type_info = other._type_info;
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
            _ptr = other._ptr;

            other._type_info = nullptr;
            other._ptr = nullptr;
        }

        proxy_storage_shared& operator=(proxy_storage_shared&& other) noexcept
        {
            std::swap(_type_info, other._type_info);
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

        [[nodiscard]] const proxy_storage_type_info* type_info() const noexcept
        {
            return _type_info;
        }

        void reset() noexcept
        {
            if (_type_info != nullptr)
            {
                if (--counter() == 0)
                {
                    SPORE_PROXY_ASSERT(_ptr != nullptr);

                    _type_info->destroy(_ptr);
                    _type_info->deallocate(_ptr);
                }

                _type_info = nullptr;
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

        const proxy_storage_type_info* _type_info;
        void* _ptr;
    };
#else
    template <typename counter_t>
    struct proxy_storage_shared
    {
        proxy_storage_shared()
            : _type_info(nullptr),
              _counter(nullptr),
              _ptr(nullptr)
        {
        }

        template <typename value_t, typename... args_t>
        explicit proxy_storage_shared(std::in_place_type_t<value_t>, args_t&&... args) SPORE_PROXY_THROW_SPEC
        {
            shared_block<value_t>* block = new shared_block<value_t> {.counter {1}, .value {std::forward<args_t>(args)...}};
            _type_info = std::addressof(proxy_storage_type_info::get<shared_block<value_t>>());
            _counter = std::addressof(block->counter);
            _ptr = std::addressof(block->value);
        }

        proxy_storage_shared(const proxy_storage_shared& other) noexcept
        {
            _type_info = other._type_info;

            if (_type_info != nullptr)
            {
                _counter = other._counter;
                _ptr = other._ptr;

                SPORE_PROXY_ASSERT(_type_info != nullptr);
                SPORE_PROXY_ASSERT(_counter != nullptr);
                SPORE_PROXY_ASSERT(_ptr != nullptr);

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
            _counter = other._counter;
            _ptr = other._ptr;

            other._type_info = nullptr;
            other._counter = nullptr;
            other._ptr = nullptr;
        }

        proxy_storage_shared& operator=(proxy_storage_shared&& other) noexcept
        {
            std::swap(_type_info, other._type_info);
            std::swap(_counter, other._counter);
            std::swap(_ptr, other._ptr);

            return *this;
        }

        ~proxy_storage_shared() noexcept
        {
            reset();
        }

        [[nodiscard]] void* ptr() const noexcept
        {
            return _ptr;
        }

        [[nodiscard]] const proxy_storage_type_info* type_info() const noexcept
        {
            return _type_info;
        }

        void reset() noexcept
        {
            if (_type_info != nullptr)
            {
                SPORE_PROXY_ASSERT(_counter != nullptr);
                SPORE_PROXY_ASSERT(_ptr != nullptr);

                if (--counter() == 0)
                {
                    _type_info->destroy(_counter);
                    _type_info->deallocate(_counter);
                }

                _type_info = nullptr;
                _counter = nullptr;
                _ptr = nullptr;
            }
        }

        counter_t& counter() noexcept
        {
            SPORE_PROXY_ASSERT(_counter != nullptr);
            return *_counter;
        }

      private:
        template <typename value_t>
        struct shared_block
        {
            counter_t counter;
            value_t value;
        };

        const proxy_storage_type_info* _type_info;
        counter_t* _counter;
        void* _ptr;
    };
#endif

    struct proxy_storage_unique
    {
        proxy_storage_unique()
            : _type_info(nullptr),
              _ptr(nullptr)
        {
        }

        template <typename storage_t>
        constexpr explicit proxy_storage_unique(storage_t&& storage) noexcept
            requires(any_proxy_storage<std::decay_t<storage_t>>)
        {
            _type_info = storage.type_info();

            if (_type_info != nullptr)
            {
                _ptr = _type_info->allocate();

                if constexpr (std::is_rvalue_reference_v<storage_t>)
                {
                    if constexpr (std::is_move_constructible_v<std::decay_t<storage_t>>)
                    {
                        _type_info->move(_ptr, storage.ptr());
                    }
                    else
                    {
                        static_assert(std::is_copy_constructible_v<std::decay_t<storage_t>>);

                        _type_info->copy(_ptr, storage.ptr());
                    }
                }
                else
                {
                    static_assert(std::is_copy_constructible_v<std::decay_t<storage_t>>);

                    _type_info->copy(_ptr, storage.ptr());
                }
            }
        }

        template <typename value_t, typename... args_t>
        explicit proxy_storage_unique(std::in_place_type_t<value_t>, args_t&&... args) SPORE_PROXY_THROW_SPEC
        {
            _type_info = std::addressof(proxies::detail::type_info<value_t>());
            _ptr = new value_t {std::forward<args_t>(args)...};
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

        [[nodiscard]] void* ptr() const noexcept
        {
            return _ptr;
        }

        [[nodiscard]] const proxy_type_info* type_info() const noexcept
        {
            return _type_info;
        }

        void reset() noexcept
        {
            if (_type_info != nullptr)
            {
                SPORE_PROXY_ASSERT(_ptr != nullptr);

                _type_info->destroy(_ptr);
                _type_info->deallocate(_ptr);

                _type_info = nullptr;
                _ptr = nullptr;
            }
        }

      private:
        const proxy_type_info* _type_info;
        void* _ptr;
    };

    struct proxy_storage_value
    {
        proxy_storage_value()
            : _type_info(nullptr),
              _ptr(nullptr)
        {
        }

        template <typename storage_t>
        constexpr explicit proxy_storage_value(storage_t&& storage) noexcept
            requires(any_proxy_storage<std::decay_t<storage_t>>)
        {
            _type_info = storage.type_info();

            if (_type_info != nullptr)
            {
                _ptr = _type_info->allocate();

                if constexpr (std::is_rvalue_reference_v<storage_t>)
                {
                    if constexpr (std::is_move_constructible_v<std::decay_t<storage_t>>)
                    {
                        _type_info->move(_ptr, storage.ptr());
                    }
                    else
                    {
                        static_assert(std::is_copy_constructible_v<std::decay_t<storage_t>>);

                        _type_info->copy(_ptr, storage.ptr());
                    }
                }
                else
                {
                    static_assert(std::is_copy_constructible_v<std::decay_t<storage_t>>);

                    _type_info->copy(_ptr, storage.ptr());
                }
            }
        }

        template <typename value_t, typename... args_t>
        explicit proxy_storage_value(std::in_place_type_t<value_t>, args_t&&... args) SPORE_PROXY_THROW_SPEC
        {
            _type_info = std::addressof(proxies::detail::type_info<value_t>());
            _ptr = _type_info->allocate();
            std::construct_at(reinterpret_cast<value_t*>(_ptr), std::forward<args_t>(args)...);
        }

        proxy_storage_value(proxy_storage_value&& other) noexcept
        {
            _type_info = other._type_info;

            if (_type_info != nullptr)
            {
                _ptr = _type_info->allocate();
                _type_info->move(ptr(), other.ptr());
            }
            else
            {
                _ptr = nullptr;
            }
        }

        proxy_storage_value(const proxy_storage_value& other) SPORE_PROXY_THROW_SPEC
        {
            _type_info = other._type_info;

            if (_type_info != nullptr)
            {
                _ptr = _type_info->allocate();
                _type_info->copy(ptr(), other.ptr());
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

            _type_info = other._type_info;

            if (_type_info != nullptr)
            {
                _ptr = _type_info->allocate();
                _type_info->move(ptr(), other.ptr());
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

            _type_info = other._type_info;

            if (_type_info != nullptr)
            {
                _ptr = _type_info->allocate();
                _type_info->copy(ptr(), other.ptr());
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

        [[nodiscard]] const proxy_type_info* type_info() const noexcept
        {
            return _type_info;
        }

        void reset() noexcept
        {
            if (_type_info != nullptr)
            {
                SPORE_PROXY_ASSERT(_ptr != nullptr);

                _type_info->destroy(_ptr);
                _type_info->deallocate(_ptr);

                _type_info = nullptr;
                _ptr = nullptr;
            }
        }

      private:
        const proxy_type_info* _type_info;
        void* _ptr;
    };

    struct proxy_storage_non_owning
    {
        proxy_storage_non_owning()
            : _type_info(nullptr),
              _ptr(nullptr)
        {
        }

        template <typename storage_t>
        constexpr explicit proxy_storage_non_owning(SPORE_PROXY_LIFETIME_BOUND storage_t&& storage) noexcept
            requires(any_proxy_storage<std::decay_t<storage_t>>)
        {
            _type_info = storage.type_info();
            _ptr = storage.ptr();
        }

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

        [[nodiscard]] constexpr void* ptr() const noexcept
        {
            return _ptr;
        }

        [[nodiscard]] constexpr const proxy_type_info* type_info() const noexcept
        {
            return _type_info;
        }

        constexpr void reset() noexcept
        {
            _type_info = nullptr;
            _ptr = nullptr;
        }

      private:
        const proxy_type_info* _type_info;
        void* _ptr;
    };

    template <std::size_t size_v, std::size_t align_v = alignof(void*)>
    struct proxy_storage_sbo
    {
        static_assert(size_v > 0);
        static_assert(align_v <= size_v);

        proxy_storage_sbo()
            : _type_info(nullptr)
        {
            std::ranges::fill(_storage, 0);
        }

        template <typename value_t, typename... args_t>
        constexpr explicit proxy_storage_sbo(std::in_place_type_t<value_t>, args_t&&... args) SPORE_PROXY_THROW_SPEC
            requires(is_compatible<value_t>())
        {
            _type_info = std::addressof(proxies::detail::type_info<value_t>());
            std::construct_at(static_cast<value_t*>(ptr()), std::forward<args_t>(args)...);
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

        [[nodiscard]] constexpr void* ptr() const noexcept
        {
            return _type_info != nullptr ? std::addressof(_storage[0]) : nullptr;
        }

        [[nodiscard]] constexpr const proxy_type_info* type_info() const noexcept
        {
            return _type_info;
        }

        void reset() noexcept
        {
            if (_type_info != nullptr)
            {
                _type_info->destroy(ptr());
                _type_info = nullptr;
            }
        }

      private:
        const proxy_type_info* _type_info;
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

        [[nodiscard]] constexpr const proxy_type_info* type_info() const noexcept
        {
            return std::addressof(proxies::detail::type_info<value_t>());
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
        constexpr explicit proxy_storage_chain(std::in_place_type_t<value_t> type, args_t&&... args) SPORE_PROXY_THROW_SPEC
        {
            std::ignore = (... or try_construct<storages_t>(type, std::forward<args_t>(args)...));
        }

        [[nodiscard]] constexpr void* ptr() const noexcept
        {
            constexpr auto visitor = [](const auto& storage) { return storage.ptr(); };
            return _storage.has_value() ? std::visit(visitor, _storage.value()) : nullptr;
        }

        [[nodiscard]] constexpr const proxy_type_info* type_info() const noexcept
        {
            constexpr auto visitor = [](const auto& storage) { return storage.type_info(); };
            return _storage.has_value() ? std::visit(visitor, _storage.value()) : nullptr;
        }

        constexpr void reset() noexcept
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