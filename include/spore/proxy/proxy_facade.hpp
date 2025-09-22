#pragma once

#include "spore/proxy/proxy_dispatch.hpp"
#include "spore/proxy/proxy_storage.hpp"

#include <type_traits>

namespace spore
{
    template <typename facade_t, typename... facades_t>
    struct proxy_facade;

    namespace proxies::detail
    {
        template <typename facade_t, typename... facades_t>
        consteval std::true_type is_proxy_facade(const proxy_facade<facade_t, facades_t...>&)
        {
            return {};
        }

        consteval std::false_type is_proxy_facade(...)
        {
            return {};
        }

        //        template <typename facade_t, typename other_facade_t, typename... other_facades_t>
        //        consteval void emplace_bases()
        //        {
        //
        //        }

        //        template <typename facade_t>
        //        consteval void emplace_bases()
        //        {
        //        }
        //
        //        template <typename facade_t, typename other_facade_t, typename... other_facades_t>
        //        consteval void emplace_bases()
        //        {
        //            proxies::detail::type_sets::emplace<proxies::detail::base_tag<facade_t>, other_facade_t>();
        //            proxies::detail::emplace_bases<facade_t, other_facades_t...>();
        //        }

        template <std::size_t index_v, typename facade_t, typename... other_facades_t>
        consteval void emplace_bases()
        {
            if constexpr (index_v < sizeof...(other_facades_t))
            {
                using other_facade_t = std::tuple_element_t<index_v, std::tuple<other_facades_t...>>;
                proxies::detail::type_sets::emplace<proxies::detail::base_tag<facade_t>, other_facade_t>();
                proxies::detail::emplace_bases<index_v + 1, facade_t, other_facades_t...>();
            }
        }
    }

    // clang-format off
    template <typename value_t>
    concept any_proxy_facade = decltype(proxies::detail::is_proxy_facade(std::declval<value_t>()))::value;
    // clang-format on

    template <typename facade_t, typename... facades_t>
    struct proxy_facade : facades_t...
    {
      protected:
        template <any_proxy_facade, any_proxy_storage>
        friend struct proxy;

        template <any_proxy_facade>
        friend struct proxy_view;

        constexpr proxy_facade() noexcept
        {
            static_assert(std::is_empty_v<facade_t> and (... and std::is_empty_v<facades_t>));
            static_assert(std::is_default_constructible_v<facade_t> and (... and std::is_default_constructible_v<facades_t>));

            // proxies::detail::emplace_bases<0, facade_t, facades_t...>();
            (proxies::detail::type_sets::emplace<proxies::detail::base_tag<facade_t>, facades_t>(), ...);
            (proxies::detail::type_sets::emplace<proxies::detail::super_tag<facades_t>, facade_t>(), ...);

#if 0
            (proxies::detail::type_sets::for_each<proxies::detail::value_tag<facade_t>>([]<typename value_t> {
                proxies::detail::init_dispatch_once<facades_t, value_t>();
            }), ...);
#else
            // new bases.
            // iterate on all values of facade and add base mappings
            proxies::detail::type_sets::for_each<proxies::detail::value_tag<facade_t>>([]<typename value_t> {
                (proxies::detail::type_sets::for_each<proxies::detail::dispatch_tag<facades_t>>([]<typename mapping_t> {
                    using func_t = typename mapping_t::func_type;
                    void* ptr = proxies::detail::get_dispatch_ptr<value_t>(mapping_t {});
                    proxy_dispatch_map::set_dispatch(proxies::detail::type_id<mapping_t>(), proxies::detail::type_id<value_t>(), ptr);
                }),
                    ...);
            });
            // proxies::detail::type_sets::for_each<proxies::detail::value_tag<facade_t>>([]<typename value_t> {
            //     proxies::detail::type_sets::emplace<proxies::detail::dispatch_tag<super_facade_t>, mapping_t>();
            // });
#endif
        }
    };
}