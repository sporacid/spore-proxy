#include "catch2/catch_all.hpp"

#include "spore/proxy/proxy.hpp"
#include "spore/proxy/tests/t_translation_unit.hpp"

#include <typeindex>

namespace spore::tests
{
    // clang-format off
    template <typename value_t>
    concept actable = requires(const value_t& value)
    {
        { value.act() };
    };
    // clang-format on

    struct facade_template : proxy_facade<facade_template>
    {
        template <typename tag_t>
        void act()
        {
            constexpr auto func = [](auto& self) { self.template act<tag_t>(); };
            proxies::dispatch(func, *this);
        }
    };

    struct impl_template
    {
        std::set<std::type_index>& type_ids;

        template <typename tag_t>
        void act()
        {
            type_ids.emplace(typeid(tag_t));
        }
    };
}

TEST_CASE("spore::proxy", "[spore::proxy]")
{
    using namespace spore;

    SECTION("basic facade")
    {
        struct facade : proxy_facade<facade>
        {
            void act()
            {
                constexpr auto func = [](auto& self) { self.act(); };
                proxies::dispatch(func, *this);
            }
        };

        struct impl
        {
            bool& flag;

            void act()
            {
                flag = true;
            }
        };

        bool flag = false;
        proxy p = proxies::make_inline<facade, impl>(flag);

        p.act();

        REQUIRE(flag);
    }

    SECTION("shared facade")
    {
        // clang-format off
        struct facade : proxy_facade<facade> {};
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

    SECTION("unique facade")
    {
        // clang-format off
        struct facade : proxy_facade<facade> {};
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

    SECTION("value facade")
    {
        struct facade : proxy_facade<facade>
        {
            [[nodiscard]] bool copied() const
            {
                constexpr auto func = [](const auto& self) { return self.copied; };
                return proxies::dispatch<bool>(func, *this);
            }
        };

        struct impl
        {
            bool* destroyed = nullptr;
            bool copied = false;

            impl() = default;

            impl(const impl&) noexcept
            {
                copied = true;
            }

            ~impl() noexcept
            {
                if (destroyed)
                {
                    *destroyed = true;
                }
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

        bool destroyed = false;
        {
            impl impl;
            impl.destroyed = std::addressof(destroyed);
            proxy _ = proxy<facade, proxy_storage_value>(std::in_place_type<decltype(impl)>, impl);
        }

        REQUIRE(destroyed);
    }

    SECTION("inline facade")
    {
        struct facade : proxy_facade<facade>
        {
            [[nodiscard]] bool copied() const
            {
                constexpr auto func = [](const auto& self) { return self.copied; };
                return proxies::dispatch<bool>(func, *this);
            }
        };

        struct impl
        {
            bool* destroyed = nullptr;
            bool copied = false;

            impl() = default;

            impl(const impl&) noexcept
            {
                copied = true;
            }

            ~impl() noexcept
            {
                if (destroyed)
                {
                    *destroyed = true;
                }
            }
        };

        static_assert(std::is_move_constructible_v<inline_proxy<facade, impl>>);
        static_assert(std::is_move_assignable_v<inline_proxy<facade, impl>>);

        static_assert(std::is_copy_constructible_v<inline_proxy<facade, impl>>);
        static_assert(std::is_copy_assignable_v<inline_proxy<facade, impl>>);

        proxy p1 = proxies::make_inline<facade, impl>();
        proxy p2 = p1;

        REQUIRE(p1.ptr() != p2.ptr());
        REQUIRE(p2.ptr() != nullptr);
        REQUIRE(p2.copied());

        const void* ptr = p1.ptr();
        proxy p3 = std::move(p1);

        REQUIRE(p1.ptr() == nullptr);
        REQUIRE(p3.ptr() == ptr);
        REQUIRE_FALSE(p3.copied());

        bool destroyed = false;
        {
            impl impl;
            impl.destroyed = std::addressof(destroyed);
            proxy _ = proxies::make_inline<facade>(impl);
        }

        REQUIRE(destroyed);
    }

    SECTION("view facade")
    {
        struct facade : proxy_facade<facade>
        {
            void act()
            {
                constexpr auto func = [](auto& self) { self.act(); };
                proxies::dispatch(func, *this);
            }
        };

        struct impl
        {
            bool& flag;

            void act()
            {
                flag = true;
            }
        };

        static_assert(std::is_move_constructible_v<inline_proxy<facade, impl>>);
        static_assert(std::is_move_assignable_v<inline_proxy<facade, impl>>);

        static_assert(std::is_copy_constructible_v<inline_proxy<facade, impl>>);
        static_assert(std::is_copy_assignable_v<inline_proxy<facade, impl>>);

        bool flag = false;
        proxy p1 = proxies::make_value<facade, impl>(flag);
        proxy_view p2 = p1;

        REQUIRE(p1.ptr() == p2.ptr());

        p2.act();

        REQUIRE(flag);
    }

    SECTION("dispatch or throw")
    {
        struct facade : proxy_facade<facade>
        {
            void act() const
            {
                constexpr auto func = []<tests::actable self_t>(const self_t& self) { self.act(); };
                proxies::dispatch_or_throw(func, *this);
            }
        };

        struct impl
        {
        };

        proxy p = proxies::make_value<facade, impl>();

        REQUIRE_THROWS_AS(p.act(), std::runtime_error);
    }

    SECTION("dispatch or default")
    {
        struct facade : proxy_facade<facade>
        {
            int act() const
            {
                constexpr auto func = []<tests::actable self_t>(const self_t& self) { self.act(); };
                return proxies::dispatch_or_default<int>(func, *this);
            }
        };

        struct impl
        {
        };

        proxy p = proxies::make_value<facade, impl>();

        int result = p.act();

        REQUIRE(result == int {});
    }

    SECTION("dispatch forward")
    {
        SECTION("dispatch forward r-value ref")
        {
            struct facade : proxy_facade<facade>
            {
                void act() &&
                {
                    constexpr auto func = []<typename self_t>(self_t&& self) { static_assert(std::is_rvalue_reference_v<self_t&&>); };
                    return proxies::dispatch(func, std::move(*this));
                }
            };

            struct impl
            {
            };

            proxy p = proxies::make_value<facade, impl>();

            std::move(p).act();
        }

        SECTION("dispatch forward l-value ref")
        {
            struct facade : proxy_facade<facade>
            {
                void act() &
                {
                    constexpr auto func = []<typename self_t>(self_t&& self) { static_assert(std::is_lvalue_reference_v<self_t>); };
                    return proxies::dispatch(func, *this);
                }
            };

            struct impl
            {
            };

            proxy p = proxies::make_value<facade, impl>();

            p.act();
        }

        SECTION("dispatch forward const ref")
        {
            struct facade : proxy_facade<facade>
            {
                void act() const
                {
                    constexpr auto func = []<typename self_t>(self_t&& self) { static_assert(std::is_const_v<std::remove_reference_t<self_t>>); };
                    return proxies::dispatch(func, *this);
                }
            };

            struct impl
            {
            };

            const proxy p = proxies::make_value<facade, impl>();

            p.act();
        }
    }

    SECTION("facade inheritance")
    {
        struct facade_base1 : proxy_facade<facade_base1>
        {
            void func1()
            {
                constexpr auto func = [](auto& self) { self.func1(); };
                proxies::dispatch(func, *this);
            }
        };

        struct facade_base2 : proxy_facade<facade_base2>
        {
            void func2()
            {
                constexpr auto func = [](auto& self) { self.func2(); };
                proxies::dispatch(func, *this);
            }
        };

        struct facade : proxy_facade<facade, facade_base1, facade_base2>
        {
        };

        struct impl
        {
            bool& flag1;
            bool& flag2;

            void func1()
            {
                flag1 = true;
            }

            void func2()
            {
                flag2 = true;
            }
        };

        bool flag1 = false;
        bool flag2 = false;

        proxy p = proxies::make_value<facade, impl>(flag1, flag2);

        p.func1();

        REQUIRE(flag1);
        REQUIRE_FALSE(flag2);

        flag1 = false;

        p.func2();

        REQUIRE(flag2);
        REQUIRE_FALSE(flag1);
    }

    SECTION("facade template")
    {
        std::set<std::type_index> type_ids;

        proxy p = proxies::make_value<tests::facade_template, tests::impl_template>(type_ids);

        // clang-format off
        struct tag1 {};
        struct tag2 {};
        // clang-format on

        p.act<tag1>();
        p.act<tag2>();

        REQUIRE(type_ids.contains(typeid(tag1)));
        REQUIRE(type_ids.contains(typeid(tag2)));
    }

    SECTION("facade across translation unit")
    {
        proxy p = proxies::tests::make_proxy();

        REQUIRE(0 == proxies::tests::some_work(p));
        REQUIRE(1 == proxies::tests::some_other_work(p));
    }
}