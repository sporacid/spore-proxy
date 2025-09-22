#pragma once

#include "spore/proxy/detail/proxy_type_id.hpp"
#include "spore/proxy/detail/proxy_type_set.hpp"
#include "spore/proxy/proxy_base.hpp"
#include "spore/proxy/proxy_macros.hpp"

#include <cstddef>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <unordered_map>

#include <fstream>
#include <iostream>

#ifndef SPORE_PROXY_DISPATCH_HEIGHT
#    define SPORE_PROXY_DISPATCH_HEIGHT static_cast<std::size_t>(32)
#endif

#ifndef SPORE_PROXY_DISPATCH_WIDTH
#    define SPORE_PROXY_DISPATCH_WIDTH static_cast<std::size_t>(1024)
#endif

namespace spore
{
    // clang-format off
    template <typename dispatch_t>
    concept any_proxy_dispatch = requires(dispatch_t& dispatch)
    {
        { dispatch.get_dispatch(static_cast<std::size_t>(0), static_cast<std::size_t>(0)) } -> std::convertible_to<void*>;
        { dispatch.set_dispatch(static_cast<std::size_t>(0), static_cast<std::size_t>(0), static_cast<void*>(nullptr)) };
    };
    // clang-format on

    struct proxy_dispatch_functional
    {
        std::function<void*(const std::size_t, const std::size_t)> get_dispatch;
        std::function<void(const std::size_t, const std::size_t, void*)> set_dispatch;
    };

    struct proxy_dispatch_map
    {
        static void* get_dispatch(const std::size_t func_id, const std::size_t type_id) noexcept
        {
            const dispatch_key dispatch_key {func_id, type_id};
            const auto it_func = _func_map.find(dispatch_key);
            return it_func != _func_map.end() ? it_func->second : nullptr;
        }

        static void set_dispatch(const std::size_t func_id, const std::size_t type_id, void* ptr) noexcept
        {
            const dispatch_key dispatch_key {func_id, type_id};
            _func_map.insert_or_assign(dispatch_key, ptr);
        }

      private:
        struct dispatch_key
        {
            std::size_t func_id;
            std::size_t type_id;

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

        static inline std::unordered_map<dispatch_key, void*, dispatch_hash> _func_map;
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

        any_proxy_dispatch auto& get_dispatch()
        {
            return dispatch_instance;
        }
    }

    namespace proxies
    {

        template <typename self_t>
        proxy_base& cast_to_proxy_base(self_t&);

        template <typename self_t>
        proxy_base&& cast_to_proxy_base(self_t&&);

        template <typename self_t>
        const proxy_base& cast_to_proxy_base(const self_t&);

        namespace detail
        {
            // one per translation unit
            // namespace
            // {
            template <typename facade_t>
            struct dispatch_tag
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

            template <typename facade_t>
            struct super_tag
            {
            };

            template <typename tag_t>
            struct once
            {
                template <typename func_t, typename... args_t>
                once(func_t&& func, args_t&&... args)
                {
                    std::call_once(
                        _once_flag,
                        std::forward<func_t>(func),
                        std::forward<args_t>(args)...);
                }

              private:
                static inline std::once_flag _once_flag;
            };
            // }

            template <typename func_t, typename self_t, typename signature_t>
            struct dispatch_mapping
            {
                using func_type = func_t;
                using self_type = self_t;
                using signature_type = signature_t;
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

            template <typename value_t, typename func_t, typename self_t, typename return_t, typename... args_t>
            void* get_dispatch_ptr(const dispatch_mapping<func_t, self_t, return_t(args_t...)>) noexcept
            {
                using void_t = std::conditional_t<std::is_const_v<std::remove_reference_t<self_t>>, const void, void>;
                static const auto func = [](void_t* ptr, args_t... args) -> return_t {
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
            }

#if 0
            template <typename facade_t, typename value_t>
            void init_dispatch_once() noexcept
            {
                struct tag;
                const bool once = [] {
                    proxies::detail::type_sets::for_each<proxies::detail::super_tag<facade_t>>([]<typename super_facade_t> {
                        proxies::detail::init_dispatch_once<super_facade_t, value_t>();
                        proxies::detail::type_sets::for_each<proxies::detail::dispatch_tag<super_facade_t>>([]<typename mapping_t> {
                            using func_t = typename mapping_t::func_type;
                            void* ptr = proxies::detail::get_dispatch_ptr<value_t>(mapping_t {});
                            proxy_dispatch_map::set_dispatch(
                                proxies::detail::type_id<func_t>(), proxies::detail::type_id<value_t>(), ptr);
                        });
                    });

#    if 0
                    proxies::detail::type_sets::for_each<proxies::detail::base_tag<facade_t>>([]<typename base_facade_t> {
                        proxies::detail::init_dispatch_once<base_facade_t, value_t>();
#        if 0
                        proxies::detail::type_sets::for_each<proxies::detail::dispatch_tag<base_facade_t>>([]<typename mapping_t> {
                            using func_t = typename mapping_t::func_type;
                            void* ptr = proxies::detail::get_dispatch_ptr<value_t>(mapping_t {});
                            proxy_dispatch_map::set_dispatch(
                                proxies::detail::type_id<func_t>(), proxies::detail::type_id<value_t>(), ptr);
                        });
#        endif
                    });

                    proxies::detail::type_sets::for_each<proxies::detail::dispatch_tag<facade_t>>([]<typename mapping_t> {
                        using func_t = typename mapping_t::func_type;
                        void* ptr = proxies::detail::get_dispatch_ptr<value_t>(mapping_t {});
                        proxy_dispatch_map::set_dispatch(
                            proxies::detail::type_id<func_t>(), proxies::detail::type_id<value_t>(), ptr);
                    });
#    endif

                    return true;
                }();

                std::ignore = once;
            }

            template <typename facade_t, typename func_t, typename self_t, typename return_t, typename... args_t>
            void init_dispatch_once(const dispatch_mapping<func_t, self_t, return_t(args_t...)>) noexcept
            {
                struct tag;
                const bool once = [] {
                    proxies::detail::type_sets::for_each<proxies::detail::value_tag<facade_t>>([]<typename value_t> {
                        // proxies::detail::init_dispatch_once<facade_t, value_t>();
                        proxies::detail::type_sets::for_each<proxies::detail::super_tag<facade_t>>([]<typename super_facade_t> {
                            proxies::detail::init_dispatch_once<super_facade_t, value_t>();
                        });
                        proxies::detail::type_sets::for_each<proxies::detail::base_tag<facade_t>>([]<typename base_facade_t> {
                            // proxies::detail::type_sets::emplace<proxies::detail::dispatch_tag<base_facade_t>, dispatch_mapping<func_t, self_t, return_t(args_t...)>>();
                            proxies::detail::init_dispatch_once<base_facade_t, value_t>();
#    if 0
                            proxies::detail::type_sets::for_each<proxies::detail::dispatch_tag<base_facade_t>>([]<typename mapping_t> {
                                using func2_t = typename mapping_t::func_type;
                                void* ptr = proxies::detail::get_dispatch_ptr<value_t>(mapping_t {});
                                proxy_dispatch_map::set_dispatch(
                                    proxies::detail::type_id<func2_t>(), proxies::detail::type_id<value_t>(), ptr);
                            });
#    endif
                        });
                    });

                    return true;
                }();

                std::ignore = once;
            }
#endif

            template <typename return_t, typename func_t, typename self_t, typename... args_t>
            constexpr return_t dispatch_impl(const func_t&, self_t&& self, args_t&&... args) SPORE_PROXY_THROW_SPEC
            {
                // constexpr std::size_t location_hash = proxies::detail::hash_source_location(location);

                using facade_t = std::decay_t<self_t>;
                using mapping_t = proxies::detail::dispatch_mapping<func_t, self_t, return_t(args_t...)>;

                static_assert(std::is_empty_v<func_t>);
                static_assert(std::is_empty_v<facade_t>);

                proxies::detail::type_sets::emplace<proxies::detail::dispatch_tag<facade_t>, mapping_t>();
#if 0
                proxies::detail::init_dispatch_once<facade_t>(mapping_t {});
#else
                // new mapping for facade, possibly a base.
                // iterate from base to super and add new mapping

                // proxies::detail::type_sets::for_each<proxies::detail::value_tag<facade_t>>([]<typename value_t> {
                //     void* ptr = proxies::detail::get_dispatch_ptr<value_t>(mapping_t {});
                //     proxy_dispatch_map::set_dispatch(proxies::detail::type_id<func_t>(), proxies::detail::type_id<value_t>(), ptr);
                // });

                proxies::detail::type_sets::for_each<proxies::detail::super_tag<facade_t>>([]<typename super_facade_t> {
                    // proxies::detail::type_sets::emplace<proxies::detail::dispatch_tag<super_facade_t>, mapping_t>();
                    proxies::detail::type_sets::for_each<proxies::detail::value_tag<super_facade_t>>([]<typename value_t> {
                        void* ptr = proxies::detail::get_dispatch_ptr<value_t>(mapping_t {});
                        proxy_dispatch_map::set_dispatch(proxies::detail::type_id<mapping_t>(), proxies::detail::type_id<value_t>(), ptr);
                    });
                });
                //                proxies::detail::type_sets::for_each<proxies::detail::base_tag<facade_t>>([]<typename super_facade_t> {
                //                    proxies::detail::type_sets::emplace<proxies::detail::dispatch_tag<super_facade_t>, mapping_t>();
                //                });

                proxies::detail::type_sets::for_each<proxies::detail::value_tag<facade_t>>([]<typename value_t> {
                    void* ptr = proxies::detail::get_dispatch_ptr<value_t>(mapping_t {});
                    proxy_dispatch_map::set_dispatch(proxies::detail::type_id<mapping_t>(), proxies::detail::type_id<value_t>(), ptr);
                });
#endif

                // using proxy_base_t = std::conditional_t<std::is_const_v<std::remove_reference_t<self_t>>, const proxy_base, proxy_base>;
                // proxy_base_t& proxy = reinterpret_cast<proxy_base_t&>(self);
                // auto&& proxy = cast_to_proxy_base<facade_t>(self);
                using proxy_base_t = std::conditional_t<std::is_const_v<std::remove_reference_t<self_t>>, const proxy_base, proxy_base>;
                using bytes_t = std::conditional_t<std::is_const_v<std::remove_reference_t<self_t>>, const std::byte, std::byte>;
                proxy_base_t& proxy = *reinterpret_cast<proxy_base_t*>(reinterpret_cast<bytes_t*>(std::addressof(self)) - sizeof(proxy_base));
                // proxy_base_t& proxy = reinterpret_cast<proxy_base_t&>(self);
                void* ptr = proxy_dispatch_map::get_dispatch(proxies::detail::type_id<mapping_t>(), proxy.type_id());

#if 0
                std::ofstream file {"C:/Dev/wtf.txt"};

                proxies::detail::type_sets::for_each<proxies::detail::dispatch_tag<facade_t>>([&]<typename mapping_t> {
                    file << typeid(mapping_t).name() << std::endl;
                    file << proxies::detail::type_id<mapping_t>() << std::endl;
                    // print_function_name<typename mapping_t::func_type>(file);
                    file << std::endl;
                });

                //                proxies::detail::type_sets::for_each<proxies::detail::value_tag<facade_t>>([&]<typename value_t> {
                //                    proxies::detail::type_sets::for_each<proxies::detail::dispatch_tag<facade_t>>([&]<typename mapping_t> {
                //                        file << typeid(value_t).name() << std::endl;
                //                        file << typeid(mapping_t).name() << std::endl;
                //                        file << proxies::detail::type_id<value_t>() << std::endl;
                //                        file << proxies::detail::type_id<typename mapping_t::func_type>() << std::endl;
                //                        file << std::endl;
                //                    });
                //                });

                proxies::detail::type_sets::for_each<proxies::detail::super_tag<facade_t>>([&]<typename super_t> {
                    // proxies::detail::type_sets::for_each<proxies::detail::value_tag<super_t>>([&]<typename value_t> {
                    proxies::detail::type_sets::for_each<proxies::detail::dispatch_tag<super_t>>([&]<typename mapping_t> {
                        // file << typeid(value_t).name() << std::endl;
                        file << typeid(mapping_t).name() << std::endl;
                        // file << proxies::detail::type_id<value_t>() << std::endl;
                        file << proxies::detail::type_id<mapping_t>() << std::endl;
                        file << std::endl;
                    });
                    // });
                });

                // file.close();

                //                std::ofstream file {"C:/Dev/wtf.txt"};
                file << typeid(facade_t).name() << std::endl;
                file << "values: " << typeid(decltype(proxies::detail::type_sets::get<proxies::detail::value_tag<facade_t>>())).name() << std::endl;
                file << "base: " << typeid(decltype(proxies::detail::type_sets::get<proxies::detail::base_tag<facade_t>>())).name() << std::endl;
                file << "super: " << typeid(decltype(proxies::detail::type_sets::get<proxies::detail::super_tag<facade_t>>())).name() << std::endl;
                file << "funcs: " << typeid(decltype(proxies::detail::type_sets::get<proxies::detail::dispatch_tag<facade_t>>())).name() << std::endl;
                file << std::endl;

                proxies::detail::type_sets::for_each<proxies::detail::super_tag<facade_t>>([&]<typename super_facade_t> {
                    file << "values: " << typeid(decltype(proxies::detail::type_sets::get<proxies::detail::value_tag<super_facade_t>>())).name() << std::endl;
                    file << "base: " << typeid(decltype(proxies::detail::type_sets::get<proxies::detail::base_tag<super_facade_t>>())).name() << std::endl;
                    file << "super: " << typeid(decltype(proxies::detail::type_sets::get<proxies::detail::super_tag<super_facade_t>>())).name() << std::endl;
                    file << "funcs: " << typeid(decltype(proxies::detail::type_sets::get<proxies::detail::dispatch_tag<super_facade_t>>())).name() << std::endl;
                    file << std::endl;
                });
                //
                file.close();
#endif

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