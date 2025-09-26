#pragma once

#include "spore/proxy/proxy_facade.hpp"
#include "spore/proxy/proxy_semantics.hpp"
#include "spore/proxy/proxy_storage.hpp"

#include <concepts>

namespace spore
{
    template <any_proxy_facade facade_t, any_proxy_storage other_storage_t, any_proxy_semantics semantics_t, any_proxy_semantics other_semantics_t>
    struct proxy_conversion<proxy<facade_t, proxy_storage_non_owning, semantics_t>, proxy<facade_t, other_storage_t, other_semantics_t>>
    {
        static constexpr void copy_proxy(proxy<facade_t, proxy_storage_non_owning, semantics_t>& proxy, const spore::proxy<facade_t, other_storage_t, other_semantics_t>& other_proxy) noexcept
        {
        }

        static constexpr void move_proxy(proxy<facade_t, proxy_storage_non_owning, semantics_t>& proxy, spore::proxy<facade_t, other_storage_t, other_semantics_t>&& other_proxy) noexcept
        {
        }
    };
}