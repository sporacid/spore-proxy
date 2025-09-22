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
    struct proxy_view : proxy_base, facade_t
    {
        constexpr proxy_view() = default;

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
    struct proxy : proxy_base, facade_t
    {
        template <typename value_t, typename... args_t>
        constexpr explicit proxy(std::in_place_type_t<value_t> type, args_t&&... args) noexcept(std::is_nothrow_constructible_v<storage_t, std::in_place_type_t<value_t>, args_t&&...>)
            : proxy_base(proxies::detail::type_id<value_t>()),
              _storage(type, std::forward<args_t>(args)...)
        {
            constexpr auto type_id = proxies::detail::type_id<value_t>();
            proxies::detail::type_sets::emplace<proxies::detail::value_tag<facade_t>, value_t>();
#if 0
            proxies::detail::init_dispatch_once<facade_t, value_t>();
#else
            // new value.
            // iterate on all mappings of facade and bases
            proxies::detail::type_sets::for_each<proxies::detail::dispatch_tag<facade_t>>([]<typename mapping_t> {
                using func_t = typename mapping_t::func_type;
                void* ptr = proxies::detail::get_dispatch_ptr<value_t>(mapping_t {});
                proxy_dispatch_map::set_dispatch(proxies::detail::type_id<mapping_t>(), proxies::detail::type_id<value_t>(), ptr);
            });

            proxies::detail::type_sets::for_each<proxies::detail::base_tag<facade_t>>([]<typename base_facade_t> {
                proxies::detail::type_sets::for_each<proxies::detail::dispatch_tag<base_facade_t>>([]<typename mapping_t> {
                    using func_t = typename mapping_t::func_type;
                    void* ptr = proxies::detail::get_dispatch_ptr<value_t>(mapping_t {});
                    proxy_dispatch_map::set_dispatch(proxies::detail::type_id<mapping_t>(), proxies::detail::type_id<value_t>(), ptr);
                });
            });

//            proxies::detail::type_sets::for_each<proxies::detail::super_tag<facade_t>>([]<typename super_facade_t> {
//                proxies::detail::type_sets::for_each<proxies::detail::dispatch_tag<super_facade_t>>([]<typename mapping_t> {
//                    using func_t = typename mapping_t::func_type;
//                    void* ptr = proxies::detail::get_dispatch_ptr<value_t>(mapping_t {});
//                    proxy_dispatch_map::set_dispatch(
//                        proxies::detail::type_id<func_t>(), proxies::detail::type_id<value_t>(), ptr);
//                });
//            });
#endif
#if 0
            proxies::detail::type_sets::for_each<proxies::detail::base_tag<facade_t>>([]<typename base_facade_t> {
                proxies::detail::init_dispatch_once<base_facade_t, value_t>();
            });
#endif

            _ptr = _storage.ptr();
        }

        constexpr proxy(const proxy& other) noexcept(std::is_nothrow_copy_constructible_v<storage_t>) requires(std::is_copy_constructible_v<storage_t>)
            : proxy_base(other._type_id)
        {
            _storage = other._storage;
            _ptr = _storage.ptr();
        }

        constexpr proxy& operator=(const proxy& other) noexcept(std::is_nothrow_copy_constructible_v<storage_t>) requires(std::is_copy_assignable_v<storage_t>)
        {
            _storage = other._storage;
            _type_id = other._type_id;
            _ptr = _storage.ptr();

            return *this;
        }

        constexpr proxy(proxy&& other) noexcept(std::is_nothrow_move_constructible_v<storage_t>) requires(std::is_move_constructible_v<storage_t>)
            : proxy_base(0)
        {
            std::swap(_storage, other._storage);
            std::swap(_type_id, other._type_id);
            std::swap(_ptr, other._ptr);
        }

        constexpr proxy& operator=(proxy&& other) noexcept(std::is_nothrow_move_constructible_v<storage_t>) requires(std::is_move_assignable_v<storage_t>)
        {
            std::swap(_storage, other._storage);
            std::swap(_type_id, other._type_id);
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

    template <any_proxy_facade facade_t>
    template <any_proxy_storage storage_t>
    constexpr proxy_view<facade_t>::proxy_view(const proxy<facade_t, storage_t>& proxy) noexcept
        : proxy_base(proxy.type_id())
    {
        _ptr = proxy.ptr();
    }

    template <any_proxy_facade facade_t>
    template <any_proxy_storage storage_t>
    constexpr proxy_view<facade_t>::proxy_view(proxy<facade_t, storage_t>& proxy) noexcept
        : proxy_base(proxy.type_id())
    {
        _ptr = proxy.ptr();
    }

    template <any_proxy_facade facade_t>
    template <any_proxy_storage storage_t>
    constexpr proxy_view<facade_t>::proxy_view(proxy<facade_t, storage_t>&& proxy) noexcept
        : proxy_base(proxy.type_id())
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
        template <typename derived_t, typename base_t>
        std::size_t get_base_offset_impl() requires std::derived_from<derived_t, base_t>
        {
            constexpr std::size_t dummy_value = 0x100000;
            const derived_t* derived_ptr = std::bit_cast<const derived_t*>(dummy_value);
            const base_t* base_ptr = static_cast<const base_t*>(derived_ptr);
            const std::size_t offset = std::bit_cast<std::size_t>(base_ptr) - dummy_value;
            return offset;
        }

        template <typename self_t>
        proxy_base& cast_to_proxy_base(self_t& self)
        {
            using facade_t = std::decay_t<self_t>;
            using proxy_t = proxy<facade_t, proxy_storage_invalid>;
            static const std::size_t offset = get_base_offset_impl<proxy_t, facade_t>();
            proxy_t& proxy = *reinterpret_cast<proxy_t*>(reinterpret_cast<std::byte*>(std::addressof(self)) + offset);
            return static_cast<proxy_base&>(proxy);
        }

        template <typename self_t>
        proxy_base&& cast_to_proxy_base(self_t&& self)
        {
            using facade_t = std::decay_t<self_t>;
            using proxy_t = proxy<facade_t, proxy_storage_invalid>;
            static const std::size_t offset = get_base_offset_impl<proxy_t, facade_t>();
            proxy_t& proxy = *reinterpret_cast<proxy_t*>(reinterpret_cast<std::byte*>(std::addressof(self)) + offset);
            return std::move(static_cast<proxy_base&>(proxy));
        }

        template <typename self_t>
        const proxy_base& cast_to_proxy_base(const self_t& self)
        {
            using facade_t = std::decay_t<self_t>;
            using proxy_t = proxy<facade_t, proxy_storage_invalid>;
            static const std::size_t offset = get_base_offset_impl<proxy_t, facade_t>();
            const proxy_t& proxy = *reinterpret_cast<const proxy_t*>(reinterpret_cast<const std::byte*>(std::addressof(self)) + offset);
            return static_cast<const proxy_base&>(proxy);
        }

        template <any_proxy_facade facade_t, typename value_t, typename... args_t>
        constexpr inline_proxy<facade_t, value_t> make_inline(args_t&&... args) noexcept(std::is_nothrow_constructible_v<inline_proxy<facade_t, value_t>, args_t&&...>)
        {
            return inline_proxy<facade_t, value_t> {std::in_place_type<value_t>, std::forward<args_t>(args)...};
        }

        template <any_proxy_facade facade_t, typename value_t>
        constexpr inline_proxy<facade_t, std::decay_t<value_t>> make_inline(value_t&& value) noexcept(std::is_nothrow_constructible_v<inline_proxy<facade_t, std::decay_t<value_t>>, value_t&&>)
        {
            return inline_proxy<facade_t, std::decay_t<value_t>> {std::in_place_type<std::decay_t<value_t>>, std::forward<value_t>(value)};
        }

        template <any_proxy_facade facade_t, typename value_t, typename... args_t>
        constexpr value_proxy<facade_t> make_value(args_t&&... args) noexcept(std::is_nothrow_constructible_v<value_proxy<facade_t>, args_t&&...>)
        {
            return value_proxy<facade_t> {std::in_place_type<value_t>, std::forward<args_t>(args)...};
        }

        template <any_proxy_facade facade_t, typename value_t>
        constexpr value_proxy<facade_t> make_value(value_t&& value) noexcept(std::is_nothrow_constructible_v<value_proxy<facade_t>, value_t&&>)
        {
            return value_proxy<facade_t> {std::in_place_type<std::decay_t<value_t>>, std::forward<value_t>(value)};
        }

        template <any_proxy_facade facade_t, typename value_t, typename... args_t>
        constexpr shared_proxy<facade_t> make_shared(args_t&&... args) noexcept(std::is_nothrow_constructible_v<shared_proxy<facade_t>, args_t&&...>)
        {
            return shared_proxy<facade_t> {std::in_place_type<value_t>, std::forward<args_t>(args)...};
        }

        template <any_proxy_facade facade_t, typename value_t>
        constexpr shared_proxy<facade_t> make_shared(value_t&& value) noexcept(std::is_nothrow_constructible_v<shared_proxy<facade_t>, value_t&&>)
        {
            return shared_proxy<facade_t> {std::in_place_type<std::decay_t<value_t>>, std::forward<value_t>(value)};
        }

        template <any_proxy_facade facade_t, typename value_t, typename... args_t>
        constexpr unique_proxy<facade_t> make_unique(args_t&&... args) noexcept(std::is_nothrow_constructible_v<unique_proxy<facade_t>, args_t&&...>)
        {
            return unique_proxy<facade_t> {std::in_place_type<value_t>, std::forward<args_t>(args)...};
        }

        template <any_proxy_facade facade_t, typename value_t>
        constexpr unique_proxy<facade_t> make_unique(value_t&& value) noexcept(std::is_nothrow_constructible_v<unique_proxy<facade_t>, value_t&&>)
        {
            return unique_proxy<facade_t> {std::in_place_type<std::decay_t<value_t>>, std::forward<value_t>(value)};
        }
    }
}