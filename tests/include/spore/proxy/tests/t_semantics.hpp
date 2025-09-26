#pragma once

#include "spore/proxy/proxy.hpp"

namespace spore::proxies::tests::semantics
{
    struct flags
    {
        bool lvalue = false;
        bool rvalue = false;
        bool const_lvalue = false;
        bool const_rvalue = false;
    };

    struct facade : proxy_facade<facade>
    {
        void act(flags& flags) &
        {
            flags.lvalue = true;
        }

        void act(flags& flags) const&
        {
            flags.const_lvalue = true;
        }

        void act(flags& flags) &&
        {
            flags.rvalue = true;
        }

        void act(flags& flags) const&&
        {
            flags.const_rvalue = true;
        }
    };

    template <template <typename> typename semantics_t>
    struct impl : semantics_t<facade>
    {
    };
}