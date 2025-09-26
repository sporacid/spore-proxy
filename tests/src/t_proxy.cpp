#include "catch2/catch_all.hpp"

#include "spore/proxy/proxy.hpp"
#include "spore/proxy/tests/t_dispatch.hpp"
#include "spore/proxy/tests/t_templates.hpp"
#include "spore/proxy/tests/t_thread.hpp"
#include "spore/proxy/tests/t_translation_unit.hpp"

#include <algorithm>
#include <array>
#include <thread>

#ifndef SPORE_PROXY_TEST_THREAD_COUNT
#    define SPORE_PROXY_TEST_THREAD_COUNT 24
#endif

TEMPLATE_TEST_CASE("spore::proxy", "[spore::proxy]", (spore::proxy_dispatch_static<>), (spore::proxy_dispatch_dynamic<>) )
{
    using namespace spore;
    using dispatch_type = TestType;

    SECTION("basic facade")
    {
        struct facade : proxy_facade<facade>
        {
            using dispatch_type = dispatch_type;

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
        struct facade : proxy_facade<facade> { using dispatch_type = dispatch_type; };
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
        struct facade : proxy_facade<facade> { using dispatch_type = dispatch_type; };
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
            using dispatch_type = dispatch_type;

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

        proxy p1 = proxies::make_value<facade, impl>();
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
            proxy _ = proxies::make_value<facade>(std::move(impl));
        }

        REQUIRE(destroyed);
    }

    SECTION("inline facade")
    {
        struct facade : proxy_facade<facade>
        {
            using dispatch_type = dispatch_type;

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

    SECTION("non-owning facade")
    {
        struct facade : proxy_facade<facade>
        {
            using dispatch_type = dispatch_type;

            void act()
            {
                constexpr auto func = [](auto& self) { self.act(); };
                proxies::dispatch(func, *this);
            }
        };

        struct impl
        {
            bool flag = false;

            void act()
            {
                flag = true;
            }
        };

        constexpr auto static_asserts = []<typename facade_t> {
            static_assert(std::is_move_constructible_v<non_owning_proxy<facade_t>>);
            static_assert(std::is_copy_constructible_v<non_owning_proxy<facade_t>>);
            static_assert(std::is_move_assignable_v<non_owning_proxy<facade_t>>);
            static_assert(std::is_copy_assignable_v<non_owning_proxy<facade_t>>);
        };

        static_asserts.template operator()<facade>();
        static_asserts.template operator()<const facade>();

        impl impl;
        proxy p = proxies::make_non_owning<facade>(impl);

        REQUIRE(p.ptr() == std::addressof(impl));

        p->act();

        REQUIRE(impl.flag);
    }

    SECTION("forward facade")
    {
        struct facade : proxy_facade<facade>
        {
            using dispatch_type = dispatch_type;

            void act()
            {
                constexpr auto func = [](auto& self) { self.act(); };
                proxies::dispatch(func, *this);
            }

            void act() const
            {
                constexpr auto func = [](auto& self) { self.act(); };
                proxies::dispatch(func, *this);
            }
        };

        struct impl
        {
            bool flag_ref = false;
            bool flag_temp_ref = false;
            mutable bool flag_const_ref = false;
            mutable bool flag_const_temp_ref = false;

            void act() &
            {
                flag_ref = true;
            }

            void act() &&
            {
                flag_temp_ref = true;
            }

            void act() const&
            {
                flag_const_ref = true;
            }

            void act() const&&
            {
                flag_const_temp_ref = true;
            }
        };

        constexpr auto static_asserts = []<typename facade_t> {
            static_assert(std::is_move_constructible_v<forward_proxy<facade_t>>);
            static_assert(std::is_copy_constructible_v<forward_proxy<facade_t>>);
            static_assert(std::is_move_assignable_v<forward_proxy<facade_t>>);
            static_assert(std::is_copy_assignable_v<forward_proxy<facade_t>>);
        };

        static_asserts.template operator()<facade>();
        static_asserts.template operator()<facade&>();
        static_asserts.template operator()<facade&&>();
        static_asserts.template operator()<const facade>();
        static_asserts.template operator()<const facade&>();
        static_asserts.template operator()<const facade&&>();

        {
            impl impl_;
            proxy p = proxies::make_forward<facade>(impl_);

            p->act();

            REQUIRE(p.ptr() == std::addressof(impl_));
            REQUIRE(impl_.flag_ref);
        }

        {
            const impl impl_;
            proxy p = proxies::make_forward<facade>(impl_);

            p->act();

            REQUIRE(p.ptr() == std::addressof(impl_));
            REQUIRE(impl_.flag_const_ref);
        }

        {
            proxy p = proxies::make_forward<facade>(impl {});

            p->act();

            impl& impl_ = *reinterpret_cast<impl*>(p.ptr());
            REQUIRE(impl_.flag_temp_ref);
        }

        {
            proxy p = proxies::make_forward<facade>([]() -> const impl { return impl {}; }());

            p->act();

            impl& impl_ = *reinterpret_cast<impl*>(p.ptr());
            REQUIRE(impl_.flag_const_temp_ref);
        }
    }

    SECTION("dispatch or throw")
    {
        struct facade : proxy_facade<facade>
        {
            using dispatch_type = dispatch_type;

            void act() const
            {
                constexpr auto func = []<proxies::tests::actable self_t>(const self_t& self) { self.act(); };
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
            using dispatch_type = dispatch_type;

            int act() const
            {
                constexpr auto func = []<proxies::tests::actable self_t>(const self_t& self) { self.act(); };
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
            using dispatch_type = dispatch_type;
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
        constexpr std::size_t result_count = 32;

        proxy p = proxies::make_value<proxies::tests::templates::facade<dispatch_type>, proxies::tests::templates::impl>();

        const auto test_result = [&]<std::size_t index_v> {
            REQUIRE(index_v == p.template act<index_v>());
        };

        const auto test_results = [&]<std::size_t... indices_v>(std::index_sequence<indices_v...>) {
            (test_result.template operator()<indices_v>(), ...);
        };

        test_results(std::make_index_sequence<result_count>());
    }

    SECTION("facade dispatch override")
    {
        struct facade : proxy_facade<facade>
        {
            using dispatch_type = proxies::tests::test_dispatch;

            void act()
            {
                constexpr auto func = [](auto& self) { self.act(); };
                proxies::dispatch(func, *this);
            }
        };

        struct impl
        {
            void act() {}
        };

        proxy p = proxies::make_unique<facade, impl>();
        p.act();

        REQUIRE(proxies::tests::test_dispatch::set_called);
        REQUIRE(proxies::tests::test_dispatch::get_called);
    }

    SECTION("facade across translation unit")
    {
        proxy p = proxies::tests::tu::make_proxy<dispatch_type>();

        REQUIRE(0 == proxies::tests::tu::some_work(p));
        REQUIRE(1 == proxies::tests::tu::some_other_work(p));
    }

    SECTION("facade across threads")
    {
        constexpr std::size_t thread_count = SPORE_PROXY_TEST_THREAD_COUNT;
        constexpr std::size_t result_count = 256;

        std::atomic_flag flag = ATOMIC_FLAG_INIT;
        std::atomic<std::size_t> counter;
        std::atomic<std::size_t> results[thread_count][result_count] {};

        proxy p = proxies::make_value<proxies::tests::threads::facade<dispatch_type>, proxies::tests::threads::impl>();

        const auto make_thread = [&]<std::size_t... indices_v>(std::index_sequence<indices_v...>) {
            return std::thread {
                [&] {
                    while (not flag.test())
                    {
                    }

                    const std::size_t thread_index = counter++;

                    const auto set_result = [&]<std::size_t index_v> {
                        results[thread_index][index_v] = p.template act<index_v>();
                    };

                    (set_result.template operator()<indices_v>(), ...);
                },
            };
        };

        const auto make_threads = [&]<std::size_t... seeds_v>(std::index_sequence<seeds_v...>) {
            return std::array<std::thread, sizeof...(seeds_v)> {
                make_thread(proxies::tests::threads::make_shuffled_index_sequence<result_count, seeds_v>())...,
            };
        };

        auto threads = make_threads(proxies::tests::threads::make_random_sequence<thread_count>());

        while (not flag.test_and_set())
        {
        }

        std::ranges::for_each(threads, [](std::thread& thread) { thread.join(); });

        for (std::size_t thread_index = 0; thread_index < thread_count; ++thread_index)
        {
            for (std::size_t result_index = 0; result_index < result_count; ++result_index)
            {
                REQUIRE(result_index == results[thread_index][result_index]);
            }
        }
    }
}