#pragma once

#include "spore/proxy/proxy_concepts.hpp"
#include "spore/proxy/proxy_macros.hpp"
#include "spore/proxy/proxy_semantics.hpp"

#include <type_traits>

namespace spore
{
    namespace proxies::detail
    {
        template <typename facade_t>
        consteval void add_facade();
    }

    template <typename facade_t, typename... facades_t>
    struct SPORE_PROXY_ENFORCE_EBCO proxy_facade : facades_t...
    {
      private:
        template <any_proxy_facade, any_proxy_storage, any_proxy_semantics>
        friend struct proxy;

        friend facade_t;

        constexpr proxy_facade() noexcept
        {
            static_assert(any_proxy_facade<facade_t> and (... and any_proxy_facade<facades_t>));

            (proxies::detail::add_facade<facade_t>());
            (proxies::detail::add_facade<facades_t>(), ...);
        }
    };
}