#include "catch2/catch_all.hpp"

#include "spore/proxy/proxy.hpp"
// #include "spore/proxy/tests/proxy_tests.hpp"

#include <iostream>

namespace spore::tests
{
}

TEST_CASE("spore::proxy", "[spore::proxy]")
{
    using namespace spore;

    SECTION("basic facade works as expected")
    {
        struct facade
        {
            void act()
            {
                constexpr auto func = [](auto& self) { self.act(); };
                proxies::dispatch(func, *this);
            }
        };

        struct impl
        {
            int& value;

            void act()
            {
                value = 1;
            }
        };

        int value = 0;

        proxy p = proxies::make_inline<facade, impl>(value);
        p.act();

        REQUIRE(value == 1);
    }

    SECTION("shared facade works as expected")
    {
        // clang-format off
        struct facade {};
        struct impl {};
        // clang-format on

        static_assert(std::is_move_constructible_v<shared_proxy<facade>>);
        static_assert(std::is_move_assignable_v<shared_proxy<facade>>);

        static_assert(std::is_copy_constructible_v<shared_proxy<facade>>);
        static_assert(std::is_copy_assignable_v<shared_proxy<facade>>);

        proxy p1 = proxies::make_shared<facade, impl>();
        proxy p2 = p1;

        REQUIRE(p1.ptr() == p2.ptr());

        proxy p3 = std::move(p2);

        REQUIRE(p1.ptr() == p3.ptr());
    }

    SECTION("unique facade works as expected")
    {
        // clang-format off
        struct facade {};
        struct impl {};
        // clang-format on

        static_assert(std::is_move_constructible_v<unique_proxy<facade>>);
        static_assert(std::is_move_assignable_v<unique_proxy<facade>>);

        static_assert(not std::is_copy_constructible_v<unique_proxy<facade>>);
        static_assert(not std::is_copy_assignable_v<unique_proxy<facade>>);

        proxy p1 = proxies::make_unique<facade, impl>();
        proxy p2 = std::move(p1);

        REQUIRE(p1.ptr() != p2.ptr());
        REQUIRE(p2.ptr() != nullptr);
        REQUIRE(p1.ptr() == nullptr);
    }

    SECTION("value facade works as expected")
    {
        struct facade
        {
            [[nodiscard]] bool copied() const
            {
                constexpr auto func = [](const auto& self) { return self.copied; };
                return proxies::dispatch<bool>(func, *this);
            }
        };

        struct impl
        {
            bool copied = false;

            impl() = default;

            impl(const impl& other)
            {
                copied = true;
            }
        };

        static_assert(std::is_move_constructible_v<value_proxy<facade>>);
        static_assert(std::is_move_assignable_v<value_proxy<facade>>);

        static_assert(std::is_copy_constructible_v<value_proxy<facade>>);
        static_assert(std::is_copy_assignable_v<value_proxy<facade>>);

        proxy p1 = proxy<facade, proxy_storage_value>(std::in_place_type<impl>);
        proxy p2 = p1;

        REQUIRE(p1.ptr() != p2.ptr());
        REQUIRE(p2.copied());

        const void* ptr = p1.ptr();
        proxy p3 = std::move(p1);

        REQUIRE(p1.ptr() == nullptr);
        REQUIRE(p3.ptr() == ptr);
    }
}