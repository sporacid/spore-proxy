#pragma once

#include "spore/proxy/detail/proxy_once.hpp"
#include "spore/proxy/detail/proxy_spin_lock.hpp"
#include "spore/proxy/detail/proxy_type_id.hpp"
#include "spore/proxy/detail/proxy_type_set.hpp"
#include "spore/proxy/proxy_base.hpp"
#include "spore/proxy/proxy_macros.hpp"

#include <array>
#include <cstddef>
#include <mutex>
#include <type_traits>
#include <unordered_map>

#ifndef SPORE_PROXY_DISPATCH_DEFAULT
#    define SPORE_PROXY_DISPATCH_DEFAULT proxy_dispatch_static<0xffff>
#endif

namespace spore
{
    // clang-format off
    // template <typename dispatch_t>
    // concept any_proxy_dispatch = requires
    // {
    //     { dispatch_t::get_ptr(static_cast<std::size_t>(0), static_cast<std::size_t>(0)) } -> std::convertible_to<void*>;
    //     { dispatch_t::set_ptr(static_cast<std::size_t>(0), static_cast<std::size_t>(0), static_cast<void*>(nullptr)) };
    // };
    // clang-format on

    template <template <typename...> typename unordered_map_t = std::unordered_map, typename mutex_t = proxies::detail::spin_lock>
    struct proxy_dispatch_dynamic
    {
        static void* get_ptr(const std::size_t mapping_id, const std::size_t type_id) noexcept
        {
            const dispatch_key dispatch_key {mapping_id, type_id};
            const std::lock_guard lock {_mutex};
            return _ptr_map[dispatch_key];
        }

        static void set_ptr(const std::size_t mapping_id, const std::size_t type_id, void* ptr) noexcept
        {
            const dispatch_key dispatch_key {mapping_id, type_id};
            const std::lock_guard lock {_mutex};
            _ptr_map[dispatch_key] = ptr;
        }

      private:
        struct dispatch_key
        {
            std::size_t mapping_id;
            std::size_t type_id;

            constexpr bool operator==(const dispatch_key& other) const
            {
                return std::tie(mapping_id, type_id) == std::tie(other.mapping_id, other.type_id);
            }
        };

        struct dispatch_hash
        {
            constexpr std::size_t operator()(const dispatch_key& key) const
            {
                return proxies::detail::hash_combine(key.type_id, key.mapping_id);
            }
        };

        static inline mutex_t _mutex;
        static inline unordered_map_t<dispatch_key, void*, dispatch_hash> _ptr_map;
    };

    template <std::size_t size_v, typename mutex_t = proxies::detail::spin_lock>
    struct proxy_dispatch_static
    {
        static void* get_ptr(const std::size_t mapping_id, const std::size_t type_id) noexcept
        {
            const std::size_t dispatch_id = proxies::detail::hash_combine(mapping_id, type_id);
            const std::uint16_t checksum = make_checksum(mapping_id, type_id);

            std::size_t dispatch_index = dispatch_id % size_v;
            std::size_t seed_index = 0;

            const std::lock_guard lock {_mutex};

            while (seed_index < std::size(_seeds))
            {
                const dispatch_ptr& ptr_data = _ptr_map[dispatch_index];

                if (not ptr_data.valid() or checksum == ptr_data.checksum())
                {
                    return ptr_data.ptr();
                }

                ++seed_index;
            }

            return nullptr;
        }

        static void set_ptr(const std::size_t mapping_id, const std::size_t type_id, void* ptr) noexcept
        {
            const std::size_t dispatch_id = proxies::detail::hash_combine(mapping_id, type_id);
            const std::uint16_t checksum = make_checksum(mapping_id, type_id);

            std::size_t dispatch_index = dispatch_id % size_v;
            std::size_t seed_index = 0;

            const std::lock_guard lock {_mutex};

            while (seed_index < std::size(_seeds))
            {
                dispatch_ptr& ptr_data = _ptr_map[dispatch_index];

                if (not ptr_data.valid())
                {
                    ptr_data = dispatch_ptr {ptr, checksum};
                    return;
                }

                ++seed_index;
            }

            SPORE_PROXY_ASSERT(false);
        }

      private:
        struct dispatch_ptr
        {
            static constexpr std::intptr_t valid_mask = 0x8000000000000000ULL;
            static constexpr std::intptr_t checksum_mask = 0x7fff000000000000ULL;
            static constexpr std::intptr_t ptr_mask = 0x0000ffffffffffffULL;

            dispatch_ptr() = default;

            dispatch_ptr(void* ptr, const std::uint16_t checksum)
            {
                _valid = static_cast<std::intptr_t>(true);
                _checksum = static_cast<std::intptr_t>(checksum) & checksum_mask;
                _ptr = std::bit_cast<std::intptr_t>(ptr) & ptr_mask;
            }

            bool valid() const noexcept
            {
                return _valid;
            }

            std::uint16_t checksum() const noexcept
            {
                return _checksum;
            }

            void* ptr() const noexcept
            {
                std::intptr_t ptr = _ptr;

#if defined(__x86_64__) || defined(_M_X64)
                constexpr std::intptr_t sign_bit = 1ULL << 47ULL;
                constexpr std::intptr_t sign_extension_bits = 0xffff000000000000;

                if (ptr & sign_bit)
                {
                    ptr |= sign_extension_bits;
                }
#endif
                return std::bit_cast<void*>(ptr);
            }

          private:
            std::intptr_t _valid : 1;
            std::intptr_t _checksum : 16;
            std::intptr_t _ptr : 48;
        };

        static inline mutex_t _mutex;
        static inline std::array<dispatch_ptr, size_v> _ptr_map {};

        static constexpr std::size_t _seeds[] {
            0x65d859d632a0a935ULL,
            0x40dc6be7c399f54aULL,
            0xa21e06cc24770a56ULL,
            0xa73a74747989357dULL,
            0xe3d482338fcde2bbULL,
            0x144bdebc9fa82bf6ULL,
            0x80feaaeb42ac942fULL,
            0xb3e9e45e5b4689f4ULL,
            0x93c836e6d3ab52a3ULL,
            0x8dc4e3c4856b16ceULL,
            0x409989a18ae7049bULL,
            0x5b547d631c3a9c8aULL,
            0xa33950eba26453edULL,
            0x9a9e29da9bde239bULL,
            0x1b2d691230396398ULL,
            0xc79bcfade4a11e6dULL,
        };

        static constexpr std::uint16_t make_checksum(const std::size_t mapping_id, const std::size_t type_id) noexcept
        {
            return static_cast<std::uint16_t>((mapping_id ^ type_id) & std::numeric_limits<std::uint16_t>::max());
        }
    };

#if 1
    struct proxy_dispatch_map
    {
        static void* get_ptr(const std::size_t func_id, const std::size_t type_id) noexcept
        {
            std::lock_guard lock {_mutex};
            const dispatch_key dispatch_key {func_id, type_id};
            const auto it_func = _func_map.find(dispatch_key);
            return it_func != _func_map.end() ? it_func->second : nullptr;
        }

        static void set_ptr(const std::size_t func_id, const std::size_t type_id, void* ptr) noexcept
        {
            std::lock_guard lock {_mutex};
            const dispatch_key dispatch_key {func_id, type_id};
            _func_map.insert_or_assign(dispatch_key, ptr);
        }

      private:
        struct dispatch_key
        {
            std::size_t func_id = 0;
            std::size_t type_id = 0;

            constexpr bool operator==(const dispatch_key& other) const
            {
                return std::tie(func_id, type_id) == std::tie(other.func_id, other.type_id);
            }
        };

        struct dispatch_hash
        {
            constexpr std::size_t operator()(const dispatch_key& key) const
            {
                return key.func_id ^ (key.func_id + 0x9e3779b9 + (key.type_id << 6) + (key.type_id >> 2));
            }
        };

        static inline std::recursive_mutex _mutex;
        static inline std::unordered_map<dispatch_key, void*, dispatch_hash> _func_map;
    };
#endif

    using proxy_dispatch_default = SPORE_PROXY_DISPATCH_DEFAULT;
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

    namespace proxies
    {
        static inline proxy_dispatch_functional dispatch_instance;

        template <any_proxy_dispatch dispatch_t>
        void set_dispatch(dispatch_t dispatch)
        {
            if constexpr (std::is_same_v<dispatch_t, proxy_dispatch_functional>)
            {
                dispatch_instance = std::move(dispatch);
            }
            else
            {
                std::shared_ptr<dispatch_t> dispatch_ptr = std::make_shared<dispatch_t>(std::move(dispatch));
                dispatch_instance = proxy_dispatch_functional {
                    .get_dispatch = std::bind(&dispatch_t::get_dispatch, dispatch_ptr),
                    .set_dispatch = std::bind(&dispatch_t::set_dispatch, dispatch_ptr),
                };
            }
        }

        inline any_proxy_dispatch auto& get_dispatch()
        {
            return dispatch_instance;
        }
    }
#endif

    namespace proxies
    {
        namespace detail
        {
            // one per translation unit
            namespace
            {
                struct facade_tag;

                template <typename...>
                struct once_tag;

                template <typename facade_t>
                struct mapping_tag
                {
                };

                template <typename facade_t>
                struct value_tag
                {
                };

                template <typename facade_t>
                struct base_tag
                {
                };
            }

            template <typename facade_t, typename func_t, typename self_t, typename signature_t>
            struct dispatch_mapping
            {
                using facade_type = facade_t;
            };

            template <typename func_t, typename return_t>
            struct dispatch_or_throw
            {
                template <typename... args_t>
                constexpr return_t operator()(args_t&&... args) const noexcept(std::is_invocable_r_v<return_t, func_t, args_t&&...>)
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

            template <typename func_t, typename return_t>
            struct dispatch_or_default
            {
                template <typename... args_t>
                constexpr return_t operator()(args_t&&... args) const noexcept(std::is_invocable_r_v<return_t, func_t, args_t&&...>)
                {
                    if constexpr (std::is_invocable_r_v<return_t, func_t, args_t&&...>)
                    {
                        return func_t {}(std::forward<args_t>(args)...);
                    }
                    else if constexpr (not std::is_void_v<return_t>)
                    {
                        static_assert(std::is_default_constructible_v<return_t>);
                        return return_t {};
                    }
                }
            };

            template <typename value_t, typename mapping_t>
            void* get_mapping_ptr() noexcept
            {
                constexpr auto unwrap_mapping = []<typename facade_t, typename func_t, typename self_t, typename return_t, typename... args_t>(const dispatch_mapping<facade_t, func_t, self_t, return_t(args_t...)>) {
                    using void_t = std::conditional_t<std::is_const_v<std::remove_reference_t<self_t>>, const void, void>;
                    const auto func = [](void_t* ptr, args_t... args) -> return_t {
                        if constexpr (std::is_const_v<std::remove_reference_t<self_t>>)
                        {
                            return func_t {}(*static_cast<const value_t*>(ptr), std::forward<args_t&&>(args)...);
                        }
                        else if constexpr (std::is_lvalue_reference_v<self_t>)
                        {
                            return func_t {}(*static_cast<value_t*>(ptr), std::forward<args_t&&>(args)...);
                        }
                        else
                        {
                            return func_t {}(std::move(*static_cast<value_t*>(ptr)), std::forward<args_t&&>(args)...);
                        }
                    };

                    return reinterpret_cast<void*>(+func);
                };

                return unwrap_mapping(mapping_t {});
            }

            template <typename facade_t>
            consteval void add_facade()
            {
                proxies::detail::type_sets::emplace<proxies::detail::facade_tag, facade_t>();
                proxies::detail::type_sets::for_each<proxies::detail::facade_tag>([]<typename other_facade_t> {
                    if constexpr (not std::is_same_v<facade_t, other_facade_t>)
                    {
                        if constexpr (std::derived_from<facade_t, other_facade_t>)
                        {
                            proxies::detail::type_sets::emplace<proxies::detail::base_tag<facade_t>, other_facade_t>();
                        }

                        if constexpr (std::derived_from<other_facade_t, facade_t>)
                        {
                            proxies::detail::type_sets::emplace<proxies::detail::base_tag<other_facade_t>, facade_t>();
                        }
                    }
                });
            }

            template <typename value_t, typename mapping_t>
            void add_value_mapping_once() noexcept
            {
                using facade_t = typename mapping_t::facade_type;
                using dispatch_t = typename facade_t::dispatch_type;
                using once_tag_t = proxies::detail::once_tag<value_t, mapping_t>;

                static const once<once_tag_t> once = [] {
                    dispatch_t::set_ptr(
                        proxies::detail::type_id<mapping_t>(),
                        proxies::detail::type_id<value_t>(),
                        proxies::detail::get_mapping_ptr<value_t, mapping_t>());
                };

                (void) once;
            }

            template <typename facade_t, typename value_t>
            void add_facade_value_once() noexcept
            {
                using once_tag_t = proxies::detail::once_tag<facade_t, value_t>;

                proxies::detail::add_facade<facade_t>();
                proxies::detail::type_sets::emplace<proxies::detail::value_tag<facade_t>, value_t>();

                static const once<once_tag_t> once = [] {
                    proxies::detail::type_sets::for_each<proxies::detail::mapping_tag<facade_t>>([]<typename mapping_t> {
                        proxies::detail::add_value_mapping_once<value_t, mapping_t>();
                    });

                    proxies::detail::type_sets::for_each<proxies::detail::base_tag<facade_t>>([]<typename base_facade_t> {
                        proxies::detail::add_facade_value_once<base_facade_t, value_t>();
                    });
                };

                (void) once;
            }

            template <typename facade_t, typename mapping_t>
            void add_facade_mapping_once() noexcept
            {
                using once_tag_t = proxies::detail::once_tag<facade_t, mapping_t>;

                proxies::detail::add_facade<facade_t>();
                proxies::detail::type_sets::emplace<proxies::detail::mapping_tag<facade_t>, mapping_t>();

                static const once<once_tag_t> once = [] {
                    proxies::detail::type_sets::for_each<proxies::detail::value_tag<facade_t>>([]<typename value_t> {
                        proxies::detail::add_value_mapping_once<value_t, mapping_t>();
                    });

                    proxies::detail::type_sets::for_each<proxies::detail::base_tag<facade_t>>([]<typename base_t> {
                        proxies::detail::type_sets::for_each<proxies::detail::value_tag<base_t>>([]<typename value_t> {
                            proxies::detail::add_value_mapping_once<value_t, mapping_t>();
                        });
                    });
                };

                (void) once;
            }

            template <typename return_t, typename func_t, typename self_t, typename... args_t>
            constexpr return_t dispatch_impl(const func_t&, self_t&& self, args_t&&... args) SPORE_PROXY_THROW_SPEC
            {
                using facade_t = std::decay_t<self_t>;
                using dispatch_t = typename facade_t::dispatch_type;
                using mapping_t = proxies::detail::dispatch_mapping<facade_t, func_t, self_t, return_t(args_t...)>;

                static_assert(std::is_empty_v<func_t>);
                static_assert(std::is_empty_v<facade_t>);

                proxies::detail::add_facade_mapping_once<facade_t, mapping_t>();

                using proxy_base_t = std::conditional_t<std::is_const_v<std::remove_reference_t<self_t>>, const proxy_base, proxy_base>;

                proxy_base_t& proxy = reinterpret_cast<proxy_base_t&>(self);
                void* ptr = dispatch_t::get_ptr(proxies::detail::type_id<mapping_t>(), proxy.type_id());

                SPORE_PROXY_ASSERT(ptr != nullptr);

                using void_t = std::conditional_t<std::is_const_v<std::remove_reference_t<self_t>>, const void, void>;
                const auto func = reinterpret_cast<return_t (*)(void_t*, args_t...)>(ptr);
                return func(proxy.ptr(), std::forward<args_t>(args)...);
            }
        }

        template <typename return_t = void, typename func_t, typename self_t, typename... args_t>
        constexpr return_t dispatch(const func_t&, self_t&& self, args_t&&... args) SPORE_PROXY_THROW_SPEC
        {
            return proxies::detail::dispatch_impl<return_t>(func_t {}, std::forward<self_t>(self), std::forward<args_t>(args)...);
        }

        template <typename return_t = void, typename func_t, typename self_t, typename... args_t>
        constexpr return_t dispatch_or_throw(const func_t&, self_t&& self, args_t&&... args) SPORE_PROXY_THROW_SPEC
        {
            return proxies::detail::dispatch_impl<return_t>(
                proxies::detail::dispatch_or_throw<func_t, return_t> {}, std::forward<self_t>(self), std::forward<args_t>(args)...);
        }

        template <typename return_t = void, typename func_t, typename self_t, typename... args_t>
        constexpr return_t dispatch_or_default(const func_t&, self_t&& self, args_t&&... args) SPORE_PROXY_THROW_SPEC
        {
            return proxies::detail::dispatch_impl<return_t>(
                proxies::detail::dispatch_or_default<func_t, return_t> {}, std::forward<self_t>(self), std::forward<args_t>(args)...);
        }
    }
}