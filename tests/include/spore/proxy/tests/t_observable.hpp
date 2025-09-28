#pragma once

namespace spore::proxies::tests::observable
{
    struct flags
    {
        static inline bool sink = false;

        bool& copied = sink;
        bool& moved = sink;
        bool& destroyed = sink;
    };

    struct impl
    {
        flags f;

        impl() = default;

        impl(flags f)
            : f(f)
        {
        }

        impl(const impl& other) noexcept
        {
            f.copied = true;
            other.f.copied = true;
        }

        impl(impl&& other) noexcept
        {
            f.moved = true;
            other.f.moved = true;
        }

        impl& operator=(const impl& other) noexcept
        {
            f.copied = true;
            other.f.copied = true;
            return *this;
        }

        impl& operator=(impl&& other) noexcept
        {
            f.moved = true;
            other.f.moved = true;
            return *this;
        }

        ~impl() noexcept
        {
            f.destroyed = true;
        }
    };

    struct base : proxy_facade<base>
    {
    };

    struct facade : proxy_facade<facade, base>
    {
        template <typename value_t>
        value_t& as()
        {
            constexpr auto f = [](auto& self) -> value_t& { return static_cast<value_t&>(self); };
            return proxies::dispatch<value_t&>(f, *this);
        }
    };
}