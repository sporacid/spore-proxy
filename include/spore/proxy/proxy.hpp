#pragma once

#include "spore/proxy/proxy_base.hpp"
#include "spore/proxy/proxy_dispatch.hpp"
#include "spore/proxy/proxy_facade.hpp"
#include "spore/proxy/proxy_semantics.hpp"
#include "spore/proxy/proxy_storage.hpp"

namespace spore
{
    template <any_proxy_facade facade_t, any_proxy_storage storage_t, any_proxy_semantics semantics_t = proxy_value_semantics<facade_t>>
    struct SPORE_PROXY_ENFORCE_EBCO proxy final : semantics_t, proxy_base
    {
        template <typename value_t, typename... args_t>
        constexpr explicit proxy(std::in_place_type_t<value_t> type, args_t&&... args) noexcept(std::is_nothrow_constructible_v<storage_t, std::in_place_type_t<value_t>, args_t&&...>)
            : proxy_base(proxies::detail::type_index<facade_t, value_t>()),
              _storage(type, std::forward<args_t>(args)...)
        {
            proxies::detail::add_facade<facade_t>();
            proxies::detail::add_facade_value_once<facade_t, value_t>();

            _ptr = _storage.ptr();
        }

        constexpr proxy(const proxy& other) noexcept(std::is_nothrow_copy_constructible_v<storage_t>) requires(std::is_copy_constructible_v<storage_t>)
            : proxy_base(other.type_index()),
              _storage(other._storage)
        {
            _ptr = _storage.ptr();
        }

        constexpr proxy& operator=(const proxy& other) noexcept(std::is_nothrow_copy_constructible_v<storage_t>) requires(std::is_copy_assignable_v<storage_t>)
        {
            _storage = other._storage;
            _type_index = other._type_index;
            _ptr = _storage.ptr();

            return *this;
        }

        constexpr proxy(proxy&& other) noexcept(std::is_nothrow_move_constructible_v<storage_t>) requires(std::is_move_constructible_v<storage_t>)
            : proxy_base(0),
              _storage(std::move(other._storage))
        {
            std::swap(_type_index, other._type_index);
            std::swap(_ptr, other._ptr);
        }

        constexpr proxy& operator=(proxy&& other) noexcept(std::is_nothrow_move_constructible_v<storage_t>) requires(std::is_move_assignable_v<storage_t>)
        {
            std::swap(_storage, other._storage);
            std::swap(_type_index, other._type_index);
            std::swap(_ptr, other._ptr);

            return *this;
        }

      private:
        storage_t _storage;
    };

    template <any_proxy_facade facade_t, typename value_t>
    using inline_proxy = proxy<facade_t, proxy_storage_inline<value_t>, proxy_value_semantics<facade_t>>;

    template <any_proxy_facade facade_t>
    using value_proxy = proxy<facade_t, proxy_storage_value, proxy_value_semantics<facade_t>>;

    template <any_proxy_facade facade_t>
    using unique_proxy = proxy<facade_t, proxy_storage_unique, proxy_value_semantics<facade_t>>;

    template <any_proxy_facade facade_t>
    using shared_proxy = proxy<facade_t, proxy_storage_shared<std::uint32_t>, proxy_pointer_semantics<facade_t>>;

    template <any_proxy_facade facade_t>
    using shared_proxy_mt = proxy<facade_t, proxy_storage_shared<std::atomic<std::uint32_t>>, proxy_pointer_semantics<facade_t>>;

    // clang-format off
    template <typename forward_facade_t> requires any_proxy_facade<std::remove_reference_t<forward_facade_t>>
    using non_owning_proxy = proxy<std::decay_t<forward_facade_t>, proxy_storage_non_owning, proxy_pointer_semantics<forward_facade_t>>;

    template <typename forward_facade_t> requires any_proxy_facade<std::decay_t<forward_facade_t>>
    using forward_proxy = proxy<std::decay_t<forward_facade_t>, proxy_storage_non_owning, proxy_forward_semantics<forward_facade_t>>;
    // clang-format on

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
        constexpr unique_proxy<facade_t> make_unique(args_t&&... args) noexcept(std::is_nothrow_constructible_v<unique_proxy<facade_t>, std::in_place_type_t<value_t>, args_t&&...>)
        {
            return unique_proxy<facade_t> {std::in_place_type<value_t>, std::forward<args_t>(args)...};
        }

        template <any_proxy_facade facade_t, typename value_t>
        constexpr unique_proxy<facade_t> make_unique(value_t&& value) noexcept(std::is_nothrow_constructible_v<unique_proxy<facade_t>, std::in_place_type_t<std::decay_t<value_t>>, value_t&&>)
        {
            return unique_proxy<facade_t> {std::in_place_type<std::decay_t<value_t>>, std::forward<value_t>(value)};
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
        constexpr shared_proxy_mt<facade_t> make_shared_mt(args_t&&... args) noexcept(std::is_nothrow_constructible_v<shared_proxy<facade_t>, std::in_place_type_t<value_t>, args_t&&...>)
        {
            return shared_proxy_mt<facade_t> {std::in_place_type<value_t>, std::forward<args_t>(args)...};
        }

        template <any_proxy_facade facade_t, typename value_t>
        constexpr shared_proxy_mt<facade_t> make_shared_mt(value_t&& value) noexcept(std::is_nothrow_constructible_v<shared_proxy<facade_t>, std::in_place_type_t<std::decay_t<value_t>>, value_t&&>)
        {
            return shared_proxy_mt<facade_t> {std::in_place_type<std::decay_t<value_t>>, std::forward<value_t>(value)};
        }

        template <any_proxy_facade facade_t, typename value_t>
        constexpr non_owning_proxy<facade_t> make_non_owning(value_t& value) noexcept
        {
            return non_owning_proxy<facade_t> {std::in_place_type<value_t>, value};
        }

        template <any_proxy_facade facade_t, typename value_t>
        constexpr non_owning_proxy<const facade_t> make_non_owning(const value_t& value) noexcept
        {
            return non_owning_proxy<const facade_t> {std::in_place_type<value_t>, value};
        }

        template <any_proxy_facade facade_t, typename value_t>
        constexpr forward_proxy<facade_t&> make_forward(value_t& value) noexcept
        {
            return forward_proxy<facade_t&> {std::in_place_type<value_t>, value};
        }

        template <any_proxy_facade facade_t, typename value_t>
        constexpr forward_proxy<facade_t&&> make_forward(value_t&& value) noexcept
        {
            return forward_proxy<facade_t&&> {std::in_place_type<std::decay_t<value_t>>, std::move(value)};
        }

        template <any_proxy_facade facade_t, typename value_t>
        constexpr forward_proxy<const facade_t&> make_forward(const value_t& value) noexcept
        {
            return forward_proxy<const facade_t&> {std::in_place_type<value_t>, value};
        }

        template <any_proxy_facade facade_t, typename value_t>
        constexpr forward_proxy<const facade_t&&> make_forward(const value_t&& value) noexcept
        {
            return forward_proxy<const facade_t&&> {std::in_place_type<std::decay_t<value_t>>, value};
        }
    }
}