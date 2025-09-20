#pragma once

#include "spore/proxy/detail/proxy_type_id.hpp"
#include "spore/proxy/detail/proxy_type_list.hpp"
#include "spore/proxy/proxy_base.hpp"
#include "spore/proxy/proxy_macros.hpp"

#include <cstddef>
#include <functional>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>

#ifndef SPORE_PROXY_DISPATCH_HEIGHT
#    define SPORE_PROXY_DISPATCH_HEIGHT static_cast<std::size_t>(32)
#endif

#ifndef SPORE_PROXY_DISPATCH_WIDTH
#    define SPORE_PROXY_DISPATCH_WIDTH static_cast<std::size_t>(1024)
#endif

namespace spore
{
    struct proxy_dispatch_key
    {
        std::size_t func_id;
        std::size_t type_id;

        bool operator==(const proxy_dispatch_key& other) const
        {
            return std::tie(func_id, type_id) == std::tie(other.func_id, other.type_id);
        }
    };
}

template <>
struct std::hash<spore::proxy_dispatch_key>
{
    constexpr std::size_t operator()(const spore::proxy_dispatch_key& key) const
    {
        return key.func_id ^ (key.func_id + 0x9e3779b9 + (key.type_id << 6) + (key.type_id >> 2));
    }
};

namespace spore
{
    struct proxy_dispatch_map
    {
        static void* get_dispatch(const std::size_t func_id, const std::size_t type_id) noexcept
        {
            const proxy_dispatch_key dispatch_key {func_id, type_id};
            const auto it_func = _func_map.find(dispatch_key);
            return it_func != _func_map.end() ? it_func->second : nullptr;
        }

        static void set_dispatch(const std::size_t func_id, const std::size_t type_id, void* ptr) noexcept
        {
            const proxy_dispatch_key dispatch_key {func_id, type_id};
            _func_map.insert_or_assign(dispatch_key, ptr);
            //            const auto it_func = _func_map.find(dispatch_key);
            //
            //            if (it_func != _func_map.end())
            //            {
            //                SPORE_PROXY_ASSERT(it_func->second == ptr);
            //            }
            //            else{
            //                _func_map.emplace(dispatch_key, )
            //            }
            //            void*& ptr_ref = _func_map[dispatch_key];
            //            // SPORE_PROXY_ASSERT(ptr_ref == nullptr or ptr_ref == ptr);
            //            ptr_ref = ptr;
        }

      private:
        static inline std::unordered_map<proxy_dispatch_key, void*> _func_map;
    };
#if 0
    struct proxy_dispatch_map
    {
        static constexpr std::size_t height = SPORE_PROXY_DISPATCH_HEIGHT;
        static constexpr std::size_t width = SPORE_PROXY_DISPATCH_WIDTH;

        static void* get_dispatch(const std::size_t func_id, const std::size_t type_id) noexcept
        {
            const std::size_t func_index = func_id & width;
            const std::size_t type_index = type_id & height;

            SPORE_PROXY_ASSERT(_func_ptrs[func_index][type_index] == nullptr or _type_ids[func_index][type_index] == type_id);

            return _func_ptrs[func_index][type_index];
        }

        static void set_dispatch(const std::size_t func_id, const std::size_t type_id, void* ptr) noexcept
        {
            const std::size_t func_index = func_id & width;
            const std::size_t type_index = type_id & height;

            SPORE_PROXY_ASSERT(_func_ptrs[func_index][type_index] == nullptr or _type_ids[func_index][type_index] == type_id);

            _func_ptrs[func_index][type_index] = ptr;
            _type_ids[func_index][type_index] = type_id;
        }

      private:
        static inline std::size_t _type_ids[width][height];
        static inline void* _func_ptrs[width][height];
    };
#endif

    namespace proxies
    {
        template <typename facade_t, typename value_t>
        void init_once();

        namespace detail
        {
            // one per translation unit
            // namespace
            // {
                template <typename facade_t>
                struct dispatch_tag {};

                template <typename facade_t>
                struct value_tag {};

                template <typename facade_t>
                struct base_tag {};
            // }

            template <typename func_t, typename self_t, typename signature_t>
            struct dispatch_mapping
            {
                using func_type = func_t;
                using self_type = self_t;
                using signature_type = signature_t;
            };

            template <typename func_t, auto = [] {}>
            struct unique_dispatch : func_t
            {
                using func_t::operator();
            };

            template <typename func_t, typename return_t>
            struct weak_dispatch
            {
                template <typename... args_t>
                constexpr return_t operator()(args_t&&... args) const SPORE_PROXY_THROW_SPEC
                {
                    if constexpr (std::is_invocable_r_v<return_t, func_t, args_t&&...>)
                    {
                        return func_t {}(std::forward<args_t>(args)...);
                    }
                    else
                    {
                        SPORE_PROXY_THROW("not callable");
                    }
                }
            };

            template <typename value_t, typename func_t, typename self_t, typename return_t, typename... args_t>
            void* get_dispatch_ptr(const dispatch_mapping<func_t, self_t, return_t(args_t...)>) noexcept
            {
                const auto func = [](void* ptr, args_t... args) -> return_t {
                    value_t& value = *static_cast<value_t*>(ptr);
                    if constexpr (std::is_const_v<self_t>)
                    {
                        return func_t {}(static_cast<const value_t&>(value), std::forward<args_t&&>(args)...);
                    }
                    else if constexpr (std::is_lvalue_reference_v<self_t>)
                    {
                        return func_t {}(value, std::forward<args_t&&>(args)...);
                    }
                    else
                    {
                        return func_t {}(std::move(value), std::forward<args_t&&>(args)...);
                    }
                };

                return reinterpret_cast<void*>(+func);
            }

            template <typename facade_t, typename value_t>
            void init_dispatch_once() noexcept
            {
                static const bool once = [] {
                    proxies::detail::type_lists::for_each<proxies::detail::base_tag<facade_t>>([]<typename base_t> {
                        proxies::detail::init_dispatch_once<base_t, value_t>();
                    });

                    proxies::detail::type_lists::for_each<proxies::detail::dispatch_tag<facade_t>>([]<typename mapping_t> {
                        using func_t = typename mapping_t::func_type;
                        void* ptr = proxies::detail::get_dispatch_ptr<value_t>(mapping_t {});
                        proxy_dispatch_map::set_dispatch(
                            proxies::detail::type_id<func_t>(), proxies::detail::type_id<value_t>(), ptr);
                    });

                    return true;
                }();

                std::ignore = once;
            }

            template <typename facade_t, typename func_t, typename self_t, typename return_t, typename... args_t>
            void init_dispatch_once(const dispatch_mapping<func_t, self_t, return_t(args_t...)>) noexcept
            {
                static const bool once = [] {
                    type_lists::for_each<proxies::detail::value_tag<facade_t>>([]<typename value_t> {
                        proxies::detail::init_dispatch_once<facade_t, value_t>();
                        type_lists::for_each<proxies::detail::base_tag<facade_t>>([]<typename base_t> {
                            proxies::detail::init_dispatch_once<base_t, value_t>();
                        });
                    });

                    return true;
                }();

                std::ignore = once;
            }

            template <typename return_t, typename func_t, typename self_t, typename... args_t>
            constexpr return_t dispatch_impl(const func_t&, self_t&& self, args_t&&... args) SPORE_PROXY_THROW_SPEC
            {
                using facade_t = std::decay_t<self_t>;
                using mapping_t = proxies::detail::dispatch_mapping<func_t, self_t, return_t(args_t...)>;

                static_assert(std::is_empty_v<func_t>);
                static_assert(std::is_empty_v<facade_t>);

                proxies::detail::type_lists::append<proxies::detail::dispatch_tag<facade_t>, mapping_t>();
                proxies::detail::init_dispatch_once<facade_t>(mapping_t {});

                const auto& proxy = reinterpret_cast<const proxy_base&>(self);
                void* ptr = proxy_dispatch_map::get_dispatch(proxies::detail::type_id<func_t>(), proxy.type_id());

                SPORE_PROXY_ASSERT(ptr != nullptr);

                const auto func = reinterpret_cast<return_t (*)(void*, args_t...)>(ptr);
                return func(proxy.ptr(), std::forward<args_t>(args)...);
            }
        }

        template <typename return_t = void, typename func_t, typename self_t, typename... args_t>
        constexpr return_t dispatch(const func_t&, self_t&& self, args_t&&... args) SPORE_PROXY_THROW_SPEC
        {
            return proxies::detail::dispatch_impl<return_t>(func_t {}, std::forward<self_t>(self), std::forward<args_t>(args)...);
        }

        template <typename return_t = void, typename func_t, typename self_t, typename... args_t>
        constexpr return_t weak_dispatch(const func_t&, self_t&& self, args_t&&... args) SPORE_PROXY_THROW_SPEC
        {
            using dispatch_t = proxies::detail::weak_dispatch<func_t, return_t>;
            return proxies::detail::dispatch_impl<return_t>(dispatch_t {}, std::forward<self_t>(self), std::forward<args_t>(args)...);
        }
    }
}