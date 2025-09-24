#pragma once

#include "spore/proxy/detail/proxy_type_id.hpp"
#include "spore/proxy/proxy_base.hpp"
#include "spore/proxy/proxy_dispatch.hpp"
#include "spore/proxy/proxy_facade.hpp"
#include "spore/proxy/proxy_storage.hpp"

namespace spore
{
    template <any_proxy_facade facade_t, any_proxy_storage storage_t>
    struct proxy;

    template <any_proxy_facade facade_t>
    struct SPORE_PROXY_ENFORCE_EBCO proxy_view final : facade_t, proxy_base
    {
        constexpr proxy_view() = default;

        template <typename value_t>
        constexpr proxy_view(value_t&& value) noexcept
            : proxy_base(proxy_dispatch_default::get_ptr_base<facade_t, std::decay_t<value_t>>(), proxies::detail::type_index<facade_t, std::decay_t<value_t>>())
        {
            proxies::detail::add_facade<facade_t>();
            proxies::detail::add_facade_value_once<facade_t, value_t>();

            if constexpr (std::is_const_v<std::remove_reference_t<value_t>>)
            {
                _ptr = const_cast<value_t*>(std::addressof(value));
            }
            else
            {
                _ptr = std::addressof(value);
            }
        }

        constexpr proxy_view(const proxy_view&) noexcept = default;
        constexpr proxy_view(proxy_view&&) noexcept = default;

        template <any_proxy_storage storage_t>
        constexpr proxy_view(SPORE_PROXY_LIFETIME_BOUND const proxy<facade_t, storage_t>& proxy) noexcept;

        template <any_proxy_storage storage_t>
        constexpr proxy_view(SPORE_PROXY_LIFETIME_BOUND proxy<facade_t, storage_t>& proxy) noexcept;

        template <any_proxy_storage storage_t>
        constexpr proxy_view(SPORE_PROXY_LIFETIME_BOUND proxy<facade_t, storage_t>&& proxy) noexcept;

        constexpr proxy_view& operator=(const proxy_view&) noexcept = default;
        constexpr proxy_view& operator=(proxy_view&&) noexcept = default;
    };

    template <any_proxy_facade facade_t, any_proxy_storage storage_t>
    struct SPORE_PROXY_ENFORCE_EBCO proxy final : facade_t, proxy_base
    {
        template <typename value_t, typename... args_t>
        constexpr explicit proxy(std::in_place_type_t<value_t> type, args_t&&... args) noexcept(std::is_nothrow_constructible_v<storage_t, std::in_place_type_t<value_t>, args_t&&...>)
            : proxy_base(proxy_dispatch_default::get_ptr_base<facade_t, value_t>(), proxies::detail::type_index<facade_t, value_t>()),
              _storage(type, std::forward<args_t>(args)...)
        {
            proxies::detail::add_facade<facade_t>();
            proxies::detail::add_facade_value_once<facade_t, value_t>();

            _ptr = _storage.ptr();
        }

        constexpr proxy(const proxy& other) noexcept(std::is_nothrow_copy_constructible_v<storage_t>) requires(std::is_copy_constructible_v<storage_t>)
            : proxy_base(other._vptr, other._type_index)
        {
            _storage = other._storage;
            _ptr = _storage.ptr();
        }

        constexpr proxy& operator=(const proxy& other) noexcept(std::is_nothrow_copy_constructible_v<storage_t>) requires(std::is_copy_assignable_v<storage_t>)
        {
            _storage = other._storage;
            _type_index = other._type_index;
            _vptr = other._vptr;
            _ptr = _storage.ptr();

            return *this;
        }

        constexpr proxy(proxy&& other) noexcept(std::is_nothrow_move_constructible_v<storage_t>) requires(std::is_move_constructible_v<storage_t>)
            : proxy_base(nullptr, 0)
        {
            std::swap(_storage, other._storage);
            std::swap(_type_index, other._type_index);
            std::swap(_vptr, other._vptr);
            std::swap(_ptr, other._ptr);
        }

        constexpr proxy& operator=(proxy&& other) noexcept(std::is_nothrow_move_constructible_v<storage_t>) requires(std::is_move_assignable_v<storage_t>)
        {
            std::swap(_storage, other._storage);
            std::swap(_type_index, other._type_index);
            std::swap(_vptr, other._vptr);
            std::swap(_ptr, other._ptr);

            return *this;
        }

        constexpr operator proxy_view<facade_t>() const& noexcept
        {
            return proxy_view<facade_t> {*this};
        }

        constexpr operator proxy_view<facade_t>() & noexcept
        {
            return proxy_view<facade_t> {*this};
        }

        constexpr operator proxy_view<facade_t>() && noexcept
        {
            return proxy_view<facade_t> {std::move(*this)};
        }

      private:
        storage_t _storage;
    };

    template <any_proxy_storage storage_t, typename value_t, typename... args_t>
    struct proxy_auto_cast
    {
        std::tuple<args_t&&...> args;

        constexpr proxy_auto_cast(args_t&&... args)
            : args(std::tie(std::forward<args_t>(args)...))
        {
        }

        template <typename facade_t>
        constexpr operator proxy<facade_t, storage_t>() && noexcept(std::is_nothrow_constructible_v<proxy<facade_t, storage_t>, std::in_place_type_t<std::decay_t<value_t>>, args_t&&...>)
        {
            return std::move(*this).template cast_to_proxy<facade_t>(std::make_index_sequence<sizeof...(args_t)>());
        }

      private:
        template <typename facade_t, std::size_t... indices_v>
        constexpr proxy<facade_t, storage_t> cast_to_proxy(std::index_sequence<indices_v...>) && noexcept(std::is_nothrow_constructible_v<proxy<facade_t, storage_t>, std::in_place_type_t<std::decay_t<value_t>>, args_t&&...>)
        {
            return proxy<facade_t, storage_t> {std::in_place_type<value_t>, std::forward<args_t>(std::get<indices_v>(args))...};
        }
    };

    template <any_proxy_facade facade_t>
    template <any_proxy_storage storage_t>
    constexpr proxy_view<facade_t>::proxy_view(const proxy<facade_t, storage_t>& proxy) noexcept
        : proxy_base(proxy.vptr(), proxy.type_index())
    {
        _ptr = proxy.ptr();
    }

    template <any_proxy_facade facade_t>
    template <any_proxy_storage storage_t>
    constexpr proxy_view<facade_t>::proxy_view(proxy<facade_t, storage_t>& proxy) noexcept
        : proxy_base(proxy.vptr(), proxy.type_index())
    {
        _ptr = proxy.ptr();
    }

    template <any_proxy_facade facade_t>
    template <any_proxy_storage storage_t>
    constexpr proxy_view<facade_t>::proxy_view(proxy<facade_t, storage_t>&& proxy) noexcept
        : proxy_base(proxy.vptr(), proxy.type_index())
    {
        _ptr = proxy.ptr();
    }

    template <any_proxy_facade facade_t, typename value_t>
    using inline_proxy = proxy<facade_t, proxy_storage_inline<sizeof(value_t), alignof(value_t)>>;

    template <any_proxy_facade facade_t>
    using value_proxy = proxy<facade_t, proxy_storage_inline_or<proxy_storage_value, 16>>;

    template <any_proxy_facade facade_t>
    using shared_proxy = proxy<facade_t, proxy_storage_shared>;

    template <any_proxy_facade facade_t>
    using unique_proxy = proxy<facade_t, proxy_storage_unique>;

    namespace proxies
    {
        template <any_proxy_facade facade_t, typename value_t, typename... args_t>
        constexpr inline_proxy<facade_t, value_t> make_inline(args_t&&... args) noexcept(std::is_nothrow_constructible_v<inline_proxy<facade_t, value_t>, std::in_place_type_t<value_t>, args_t&&...>)
        {
            return inline_proxy<facade_t, value_t> {std::in_place_type<value_t>, std::forward<args_t>(args)...};
        }

        template <any_proxy_facade facade_t, typename value_t>
        constexpr inline_proxy<facade_t, std::decay_t<value_t>> make_inline(value_t&& value) noexcept(std::is_nothrow_constructible_v<inline_proxy<facade_t, std::decay_t<value_t>>, std::in_place_type_t<std::decay_t<value_t>>, value_t&&>)
        {
            return inline_proxy<facade_t, std::decay_t<value_t>> {std::in_place_type<std::decay_t<value_t>>, std::forward<value_t>(value)};
        }

        template <any_proxy_facade facade_t, typename value_t, typename... args_t>
        constexpr value_proxy<facade_t> make_value(args_t&&... args) noexcept(std::is_nothrow_constructible_v<value_proxy<facade_t>, std::in_place_type_t<value_t>, args_t&&...>)
        {
            return value_proxy<facade_t> {std::in_place_type<value_t>, std::forward<args_t>(args)...};
        }

        template <any_proxy_facade facade_t, typename value_t>
        constexpr value_proxy<facade_t> make_value(value_t&& value) noexcept(std::is_nothrow_constructible_v<value_proxy<facade_t>, std::in_place_type_t<std::decay_t<value_t>>, value_t&&>)
        {
            return value_proxy<facade_t> {std::in_place_type<std::decay_t<value_t>>, std::forward<value_t>(value)};
        }

        template <any_proxy_facade facade_t, typename value_t, typename... args_t>
        constexpr shared_proxy<facade_t> make_shared(args_t&&... args) noexcept(std::is_nothrow_constructible_v<shared_proxy<facade_t>, std::in_place_type_t<value_t>, args_t&&...>)
        {
            return shared_proxy<facade_t> {std::in_place_type<value_t>, std::forward<args_t>(args)...};
        }

        template <any_proxy_facade facade_t, typename value_t>
        constexpr shared_proxy<facade_t> make_shared(value_t&& value) noexcept(std::is_nothrow_constructible_v<shared_proxy<facade_t>, std::in_place_type_t<std::decay_t<value_t>>, value_t&&>)
        {
            return shared_proxy<facade_t> {std::in_place_type<std::decay_t<value_t>>, std::forward<value_t>(value)};
        }

        template <any_proxy_facade facade_t, typename value_t, typename... args_t>
        constexpr unique_proxy<facade_t> make_unique(args_t&&... args) noexcept(std::is_nothrow_constructible_v<unique_proxy<facade_t>, std::in_place_type_t<value_t>, args_t&&...>)
        {
            return unique_proxy<facade_t> {std::in_place_type<value_t>, std::forward<args_t>(args)...};
        }

        template <any_proxy_facade facade_t, typename value_t>
        constexpr unique_proxy<facade_t> make_unique(value_t&& value) noexcept(std::is_nothrow_constructible_v<unique_proxy<facade_t>, std::in_place_type_t<std::decay_t<value_t>>, value_t&&>)
        {
            return unique_proxy<facade_t> {std::in_place_type<std::decay_t<value_t>>, std::forward<value_t>(value)};
        }

        template <typename value_t, typename... args_t>
        constexpr auto make_unique2(args_t&&... args)
        {
            return proxy_auto_cast<proxy_storage_unique, value_t> {std::forward<args_t>(args)...};
        }

        template <typename value_t>
        constexpr auto make_unique2(value_t&& value)
        {
            using decay_value_t = std::decay_t<value_t>;
            return proxy_auto_cast<proxy_storage_unique, decay_value_t> {std::forward<value_t>(value)};
        }
    }
}