#pragma once

#include "spore/proxy/proxy_version.hpp"

#ifdef SPORE_PROXY_NO_THROW
#    include <cstdio>
#    include <cstdlib>

#    define SPORE_PROXY_THROW_SPEC noexcept()
#    define SPORE_PROXY_THROW(Message) \
        do                             \
        {                              \
            std::puts("{}", Message);  \
            std::abort();              \
        } while (false)
#else
#    ifndef SPORE_PROXY_THROW
#        include <stdexcept>
#        define SPORE_PROXY_THROW(Message) throw std::runtime_error((Message))
#    endif

#    ifndef SPORE_PROXY_THROW_SPEC
#        define SPORE_PROXY_THROW_SPEC
#    endif
#endif

#ifndef SPORE_PROXY_ASSERT
#    ifdef _DEBUG
#        define SPORE_PROXY_ASSERT(Cond)                                       \
            do                                                                 \
            {                                                                  \
                if (!(Cond)) [[unlikely]]                                      \
                {                                                              \
                    std::puts("[" SPORE_PROXY_NAME "] assert failed: " #Cond); \
                    std::abort();                                              \
                }                                                              \
            } while (false)
#    else
#        define SPORE_PROXY_ASSERT(Cond)
#    endif
#endif