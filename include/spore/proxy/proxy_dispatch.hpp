#pragma once

#include "spore/proxy/proxy_base.hpp"
#include "spore/proxy/proxy_facade.hpp"
#include "spore/proxy/proxy_macros.hpp"
#include "spore/proxy/proxy_type_set.hpp"

#include <cstddef>
#include <type_traits>
#include <vector>

#ifndef SPORE_PROXY_DISPATCH_DEFAULT
#    define SPORE_PROXY_DISPATCH_DEFAULT proxy_dispatch_dynamic<>
#endif

namespace spore
{
    namespace proxies::detail
    {
        template <typename facade_t, typename func_t, typename self_t, typename signature_t>
        struct dispatch_mapping;

        template <typename facade_t, typename func_t, typename self_t, typename return_t, typename... args_t>
        struct dispatch_mapping<facade_t, func_t, self_t, return_t(args_t...)>
        {
            using facade_type = facade_t;
            using void_type = std::conditional_t<std::is_const_v<std::remove_reference_t<self_t>>, const void, void>;
            using dispatch_type = return_t (*)(void_type*, args_t&&...);

            template <typename value_t>
            SPORE_PROXY_FORCE_INLINE static constexpr return_t dispatch(void_type* ptr, args_t&&... args) SPORE_PROXY_THROW_SPEC
            {
                if constexpr (std::is_const_v<std::remove_reference_t<self_t>>)
                {
                    return func_t {}(*static_cast<const value_t*>(ptr), std::forward<args_t>(args)...);
                }
                else if constexpr (std::is_lvalue_reference_v<self_t>)
                {
                    return func_t {}(*static_cast<value_t*>(ptr), std::forward<args_t>(args)...);
                }
                else
                {
                    return func_t {}(std::move(*static_cast<value_t*>(ptr)), std::forward<args_t>(args)...);
                }
            }
        };

        template <typename mapping_t>
        using dispatch_type = typename mapping_t::dispatch_type;

        template <typename tag_t>
        struct index_impl
        {
            static inline std::atomic<std::uint32_t> seed;

            template <typename value_t>
            static inline std::uint32_t value = seed++;
        };

        template <typename facade_t>
        struct type_index_tag;

        template <typename facade_t, typename value_t>
        std::uint32_t type_index()
        {
            return index_impl<type_index_tag<facade_t>>::template value<value_t>;
        }
    }

    template <std::size_t size_v = 16, std::float_t grow_v = 1.5f>
    struct [[maybe_unused]] proxy_dispatch_dynamic
    {
        template <typename mapping_t>
        static inline thread_local std::vector<proxies::detail::dispatch_type<mapping_t>> dispatches {size_v};

        template <typename tag_t, typename func_t>
        SPORE_PROXY_FORCE_INLINE static void call_once(const func_t&)
        {
            static_assert(std::is_empty_v<func_t>);
            static_assert(std::is_default_constructible_v<func_t>);
            [[maybe_unused]] static thread_local const bool once = [] {
                func_t {}();
                return true;
            }();
        }

        template <typename mapping_t>
        [[nodiscard]] SPORE_PROXY_FORCE_INLINE static constexpr proxies::detail::dispatch_type<mapping_t> get_dispatch(const std::uint32_t type_index) noexcept
        {
            const std::vector<proxies::detail::dispatch_type<mapping_t>>& mapping_dispatchers = dispatches<mapping_t>;

            if (type_index < mapping_dispatchers.size()) [[likely]]
            {
                return mapping_dispatchers[type_index];
            }

            return nullptr;
        }

        template <typename mapping_t>
        SPORE_PROXY_FORCE_INLINE static void set_dispatch(const std::uint32_t type_index, const proxies::detail::dispatch_type<mapping_t> dispatcher) noexcept
        {
            std::vector<proxies::detail::dispatch_type<mapping_t>>& mapping_dispatches = dispatches<mapping_t>;

            if (type_index >= mapping_dispatches.size()) [[unlikely]]
            {
                const std::size_t new_size = std::max<std::size_t>(type_index + 1, mapping_dispatches.size() * grow_v);
                mapping_dispatches.resize(new_size);
            }

            std::construct_at(std::addressof(mapping_dispatches[type_index]), dispatcher);
        }
    };

    template <std::size_t size_v = 64>
    struct [[maybe_unused]] proxy_dispatch_static
    {
        template <typename mapping_t>
        static inline proxies::detail::dispatch_type<mapping_t> dispatches[size_v] {};

        template <typename tag_t, typename func_t>
        SPORE_PROXY_FORCE_INLINE static void call_once(const func_t&)
        {
            static_assert(std::is_empty_v<func_t>);
            static_assert(std::is_default_constructible_v<func_t>);
            [[maybe_unused]] static const bool once = [] {
                func_t {}();
                return true;
            }();
        }

        template <typename mapping_t>
        [[nodiscard]] SPORE_PROXY_FORCE_INLINE static constexpr proxies::detail::dispatch_type<mapping_t> get_dispatch(const std::uint32_t type_index) noexcept
        {
            SPORE_PROXY_ASSERT(type_index < size_v);
            return dispatches<mapping_t>[type_index];
        }

        template <typename mapping_t>
        SPORE_PROXY_FORCE_INLINE static void set_dispatch(const std::uint32_t type_index, const proxies::detail::dispatch_type<mapping_t> dispatcher) noexcept
        {
            SPORE_PROXY_ASSERT(type_index < size_v);
            std::construct_at(std::addressof(dispatches<mapping_t>[type_index]), dispatcher);
        }
    };

    using proxy_dispatch_default = SPORE_PROXY_DISPATCH_DEFAULT;

    namespace proxies
    {
        namespace detail
        {
            // one per translation unit
            namespace
            {
                struct facade_tag;

                template <typename... tags_t>
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

            // clang-format off
            template <typename facade_t>
            concept facade_dispatch_type_override = requires
            {
                { std::in_place_type<typename facade_t::dispatch_type> };
            };
            // clang-format on

            template <typename facade_t>
            struct select_dispatch_type
            {
                using type = proxy_dispatch_default;
            };

            template <facade_dispatch_type_override facade_t>
            struct select_dispatch_type<facade_t>
            {
                using type = typename facade_t::dispatch_type;
            };

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
            SPORE_PROXY_FORCE_INLINE void add_value_mapping_once() noexcept
            {
                using tag_t = proxies::detail::once_tag<value_t, mapping_t>;
                using facade_t = typename mapping_t::facade_type;
                using dispatch_t = typename select_dispatch_type<facade_t>::type;
                dispatch_t::template call_once<tag_t>([] {
                    const std::uint32_t type_index = proxies::detail::type_index<facade_t, value_t>();
                    dispatch_t::template set_dispatch<mapping_t>(type_index, &mapping_t::template dispatch<value_t>);
                });
            }

            template <typename facade_t, typename value_t>
            SPORE_PROXY_FORCE_INLINE void add_facade_value_once() noexcept
            {
                using tag_t = proxies::detail::once_tag<facade_t, value_t>;
                using dispatch_t = typename select_dispatch_type<facade_t>::type;
                dispatch_t::template call_once<tag_t>([] {
                    proxies::detail::add_facade<facade_t>();
                    proxies::detail::type_sets::emplace<proxies::detail::value_tag<facade_t>, value_t>();

                    proxies::detail::type_sets::for_each<proxies::detail::mapping_tag<facade_t>>([]<typename mapping_t> {
                        proxies::detail::add_value_mapping_once<value_t, mapping_t>();
                    });

                    proxies::detail::type_sets::for_each<proxies::detail::base_tag<facade_t>>([]<typename base_facade_t> {
                        proxies::detail::add_facade_value_once<base_facade_t, value_t>();
                    });
                });
            }

            template <typename facade_t, typename mapping_t>
            SPORE_PROXY_FORCE_INLINE void add_facade_mapping_once() noexcept
            {
                using tag_t = proxies::detail::once_tag<facade_t, mapping_t>;
                using dispatch_t = typename select_dispatch_type<facade_t>::type;
                dispatch_t::template call_once<tag_t>([] {
                    proxies::detail::add_facade<facade_t>();
                    proxies::detail::type_sets::emplace<proxies::detail::mapping_tag<facade_t>, mapping_t>();

                    proxies::detail::type_sets::for_each<proxies::detail::value_tag<facade_t>>([]<typename value_t> {
                        proxies::detail::add_value_mapping_once<value_t, mapping_t>();
                    });

                    proxies::detail::type_sets::for_each<proxies::detail::base_tag<facade_t>>([]<typename base_t> {
                        proxies::detail::type_sets::for_each<proxies::detail::value_tag<base_t>>([]<typename value_t> {
                            proxies::detail::add_value_mapping_once<value_t, mapping_t>();
                        });
                    });
                });
            }

            template <typename return_t, typename func_t, typename self_t, typename... args_t>
            constexpr return_t dispatch_impl(const func_t&, self_t&& self, args_t&&... args) SPORE_PROXY_THROW_SPEC
            {
                using facade_t = std::decay_t<self_t>;
                using dispatch_t = typename select_dispatch_type<facade_t>::type;
                using mapping_t = proxies::detail::dispatch_mapping<facade_t, func_t, self_t, return_t(args_t...)>;

                static_assert(std::is_empty_v<func_t>);
                static_assert(std::is_empty_v<facade_t>);

                proxies::detail::add_facade_mapping_once<facade_t, mapping_t>();

                using proxy_base_t = std::conditional_t<std::is_const_v<std::remove_reference_t<self_t>>, const proxy_base, proxy_base>;
                proxy_base_t& proxy = reinterpret_cast<proxy_base_t&>(self);

                const auto dispatch = dispatch_t::template get_dispatch<mapping_t>(proxy.type_index());
                SPORE_PROXY_ASSERT(dispatch != nullptr);

                return dispatch(proxy.ptr(), std::forward<args_t>(args)...);
            }
        }

        template <typename return_t = void, typename func_t, typename self_t, typename... args_t>
        SPORE_PROXY_FORCE_INLINE constexpr return_t dispatch(const func_t& func, self_t&& self, args_t&&... args) SPORE_PROXY_THROW_SPEC
        {
            return proxies::detail::dispatch_impl<return_t>(func, std::forward<self_t>(self), std::forward<args_t>(args)...);
        }

        template <typename return_t = void, typename func_t, typename self_t, typename... args_t>
        SPORE_PROXY_FORCE_INLINE constexpr return_t dispatch_or_throw(const func_t&, self_t&& self, args_t&&... args) SPORE_PROXY_THROW_SPEC
        {
            return proxies::detail::dispatch_impl<return_t>(
                proxies::detail::dispatch_or_throw<func_t, return_t> {}, std::forward<self_t>(self), std::forward<args_t>(args)...);
        }

        template <typename return_t = void, typename func_t, typename self_t, typename... args_t>
        SPORE_PROXY_FORCE_INLINE constexpr return_t dispatch_or_default(const func_t&, self_t&& self, args_t&&... args) SPORE_PROXY_THROW_SPEC
        {
            return proxies::detail::dispatch_impl<return_t>(
                proxies::detail::dispatch_or_default<func_t, return_t> {}, std::forward<self_t>(self), std::forward<args_t>(args)...);
        }
    }
}
