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
#        include <format>
#        include <stdexcept>

#        define SPORE_PROXY_THROW(Message) throw std::runtime_error(std::format("[" SPORE_PROXY_NAME "] {}", (Message)))
#    endif

#    ifndef SPORE_PROXY_THROW_SPEC
#        define SPORE_PROXY_THROW_SPEC
#    endif
#endif

#ifndef SPORE_PROXY_ASSERT
#    ifdef _DEBUG
#        include <cstdio>
#        include <cstdlib>

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
#        define SPORE_PROXY_ASSERT(Cond) (void) 0
#    endif
#endif

#if defined(_MSC_VER)
#    define SPORE_PROXY_ENFORCE_EBCO __declspec(empty_bases)
#    define SPORE_PROXY_ENFORCE_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#else
#    define SPORE_PROXY_ENFORCE_EBCO
#    define SPORE_PROXY_ENFORCE_NO_UNIQUE_ADDRESS [[no_unique_address]]
#endif

#if defined(_MSC_VER) && !defined(__clang__) && _MSC_VER >= 1937
#    define SPORE_PROXY_LIFETIME_BOUND [[msvc::lifetimebound]]
#else
#    define SPORE_PROXY_LIFETIME_BOUND
#endif

#if defined(_MSC_VER)
#    define SPORE_PROXY_FORCE_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#    define SPORE_PROXY_FORCE_INLINE __attribute__((always_inline))
#else
#    define SPORE_PROXY_FORCE_INLINE
#endif