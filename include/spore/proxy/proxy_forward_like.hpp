#pragma once

#ifdef __cpp_lib_forward_like
#    include <utility>
#else
#    include <type_traits>

namespace std
{
    template <typename like_t, typename value_t>
    constexpr decltype(auto) forward_like(value_t&& x) noexcept
    {
        if constexpr (std::is_lvalue_reference_v<like_t>)
        {
            if constexpr (std::is_const_v<std::remove_reference_t<like_t>>)
            {
                return static_cast<const std::remove_reference_t<value_t>&>(x);
            }
            else
            {
                return static_cast<std::remove_reference_t<value_t>&>(x);
            }
        }
        else
        {
            if constexpr (std::is_const_v<std::remove_reference_t<like_t>>)
            {
                return static_cast<const std::remove_reference_t<value_t>&&>(x);
            }
            else
            {
                return static_cast<std::remove_reference_t<value_t>&&>(x);
            }
        }
    }
}
#endif