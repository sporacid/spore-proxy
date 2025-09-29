#include "catch2/catch_all.hpp"

#define private public

#include "spore/proxy/proxy.hpp"
#include "spore/proxy/tests/t_conversions.hpp"
#include "spore/proxy/tests/t_dispatch.hpp"
#include "spore/proxy/tests/t_observable.hpp"
#include "spore/proxy/tests/t_templates.hpp"
#include "spore/proxy/tests/t_thread.hpp"
#include "spore/proxy/tests/t_translation_unit.hpp"

#include <algorithm>
#include <array>
#include <thread>

#ifndef SPORE_PROXY_TEST_THREAD_COUNT
#    define SPORE_PROXY_TEST_THREAD_COUNT 24
#endif

namespace spore::proxies::tests
{
    namespace static_asserts
    {
        // clang-format off
        struct facade : proxy_facade<facade> {};
        struct impl {};
        // clang-format on

        static_assert(std::is_move_constructible_v<shared_proxy<facade>>);
        static_assert(std::is_copy_constructible_v<shared_proxy<facade>>);
        static_assert(std::is_move_assignable_v<shared_proxy<facade>>);
        static_assert(std::is_copy_assignable_v<shared_proxy<facade>>);

        static_assert(std::is_move_constructible_v<unique_proxy<facade>>);
        static_assert(not std::is_copy_constructible_v<unique_proxy<facade>>);
        static_assert(std::is_move_assignable_v<unique_proxy<facade>>);
        static_assert(not std::is_copy_assignable_v<unique_proxy<facade>>);

        static_assert(std::is_move_constructible_v<value_proxy<facade>>);
        static_assert(std::is_copy_constructible_v<value_proxy<facade>>);
        static_assert(std::is_move_assignable_v<value_proxy<facade>>);
        static_assert(std::is_copy_assignable_v<value_proxy<facade>>);

        static_assert(std::is_move_constructible_v<inline_proxy<facade, impl>>);
        static_assert(std::is_copy_constructible_v<inline_proxy<facade, impl>>);
        static_assert(std::is_move_assignable_v<inline_proxy<facade, impl>>);
        static_assert(std::is_copy_assignable_v<inline_proxy<facade, impl>>);

#define SPORE_PROXY_TESTS_STATIC_ASSERTS(Proxy, Facade)         \
    static_assert(std::is_move_constructible_v<Proxy<Facade>>); \
    static_assert(std::is_copy_constructible_v<Proxy<Facade>>); \
    static_assert(std::is_move_assignable_v<Proxy<Facade>>);    \
    static_assert(std::is_copy_assignable_v<Proxy<Facade>>)

        SPORE_PROXY_TESTS_STATIC_ASSERTS(view_proxy, facade);
        SPORE_PROXY_TESTS_STATIC_ASSERTS(view_proxy, const facade);

        SPORE_PROXY_TESTS_STATIC_ASSERTS(forward_proxy, facade&);
        SPORE_PROXY_TESTS_STATIC_ASSERTS(forward_proxy, facade&&);
        SPORE_PROXY_TESTS_STATIC_ASSERTS(forward_proxy, const facade&);
        SPORE_PROXY_TESTS_STATIC_ASSERTS(forward_proxy, const facade&&);

#undef SPORE_PROXY_TESTS_STATIC_ASSERTS
    }
}

TEMPLATE_TEST_CASE("spore::proxy", "[spore::proxy]", (spore::proxy_dispatch_static<>), (spore::proxy_dispatch_dynamic<>) )
{
    using namespace spore;

    SECTION("proxy basics")
    {
        struct facade : proxy_facade<facade>
        {
            using dispatch_type [[maybe_unused]] = TestType;

            void act()
            {
                proxies::dispatch(*this, [](auto& self) { self.act(); });
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

    SECTION("proxy conversions")
    {
        SECTION("static assertions")
        {
            using namespace proxies::tests::conversions;

            // value semantics conversion

            static_assert_conversion<value_proxy<facade>, value_proxy<facade>, copy::enabled, move::enabled>();
            static_assert_conversion<value_proxy<facade>, value_proxy<base>, copy::disabled, move::disabled>();
            static_assert_conversion<value_proxy<base>, value_proxy<facade>, copy::enabled, move::enabled>();

            // pointer semantics conversion

            static_assert_conversion<view_proxy<facade>, view_proxy<facade>, copy::enabled, move::disabled>();
            static_assert_conversion<view_proxy<facade>, view_proxy<const facade>, copy::disabled, move::disabled>();
            static_assert_conversion<view_proxy<const facade>, view_proxy<facade>, copy::enabled, move::disabled>();
            static_assert_conversion<view_proxy<const facade>, view_proxy<const facade>, copy::enabled, move::disabled>();

            static_assert_conversion<view_proxy<base>, view_proxy<facade>, copy::enabled, move::disabled>();
            static_assert_conversion<view_proxy<base>, view_proxy<const facade>, copy::disabled, move::disabled>();
            static_assert_conversion<view_proxy<const base>, view_proxy<facade>, copy::enabled, move::disabled>();
            static_assert_conversion<view_proxy<const base>, view_proxy<const facade>, copy::enabled, move::disabled>();

            static_assert_conversion<view_proxy<facade>, value_proxy<facade>, copy::enabled, move::enabled>();
            static_assert_conversion<view_proxy<base>, value_proxy<facade>, copy::enabled, move::enabled>();
            static_assert_conversion<view_proxy<const facade>, value_proxy<facade>, copy::enabled, move::enabled>();
            static_assert_conversion<view_proxy<const base>, value_proxy<facade>, copy::enabled, move::enabled>();

            // reference semantics conversions

            static_assert_conversion<forward_proxy<facade&>, forward_proxy<facade&>, copy::enabled, move::disabled>();
            static_assert_conversion<forward_proxy<facade&>, forward_proxy<facade&&>, copy::enabled, move::enabled>();
            static_assert_conversion<forward_proxy<facade&>, forward_proxy<const facade&>, copy::enabled, move::disabled>();
            static_assert_conversion<forward_proxy<facade&>, forward_proxy<const facade&&>, copy::enabled, move::enabled>();
            static_assert_conversion<forward_proxy<facade&>, view_proxy<facade>, copy::enabled, move::disabled>();
            static_assert_conversion<forward_proxy<facade&>, view_proxy<const facade>, copy::enabled, move::disabled>();
            static_assert_conversion<forward_proxy<facade&>, value_proxy<facade>, copy::enabled, move::enabled>();
            static_assert_conversion<forward_proxy<base&>, forward_proxy<facade&>, copy::enabled, move::disabled>();

            static_assert_conversion<forward_proxy<facade&&>, forward_proxy<facade&>, copy::enabled, move::disabled>();
            static_assert_conversion<forward_proxy<facade&&>, forward_proxy<facade&&>, copy::enabled, move::enabled>();
            static_assert_conversion<forward_proxy<facade&&>, forward_proxy<const facade&>, copy::enabled, move::disabled>();
            static_assert_conversion<forward_proxy<facade&&>, forward_proxy<const facade&&>, copy::enabled, move::enabled>();
            static_assert_conversion<forward_proxy<facade&&>, view_proxy<facade>, copy::enabled, move::disabled>();
            static_assert_conversion<forward_proxy<facade&&>, view_proxy<const facade>, copy::enabled, move::disabled>();
            static_assert_conversion<forward_proxy<facade&&>, value_proxy<facade>, copy::enabled, move::enabled>();
            static_assert_conversion<forward_proxy<base&&>, forward_proxy<facade&&>, copy::enabled, move::enabled>();

            static_assert_conversion<forward_proxy<const facade&>, forward_proxy<facade&>, copy::disabled, move::disabled>();
            static_assert_conversion<forward_proxy<const facade&>, forward_proxy<facade&&>, copy::disabled, move::disabled>();
            static_assert_conversion<forward_proxy<const facade&>, forward_proxy<const facade&>, copy::disabled, move::disabled>();
            static_assert_conversion<forward_proxy<const facade&>, forward_proxy<const facade&&>, copy::disabled, move::disabled>();
            static_assert_conversion<forward_proxy<const facade&>, view_proxy<facade>, copy::disabled, move::disabled>();
            static_assert_conversion<forward_proxy<const facade&>, view_proxy<const facade>, copy::disabled, move::disabled>();
            static_assert_conversion<forward_proxy<const facade&>, value_proxy<facade>, copy::disabled, move::disabled>();
            static_assert_conversion<forward_proxy<const base&>, forward_proxy<const facade&>, copy::disabled, move::disabled>();

            static_assert_conversion<forward_proxy<const facade&&>, forward_proxy<facade&>, copy::disabled, move::disabled>();
            static_assert_conversion<forward_proxy<const facade&&>, forward_proxy<facade&&>, copy::disabled, move::disabled>();
            static_assert_conversion<forward_proxy<const facade&&>, forward_proxy<const facade&>, copy::disabled, move::disabled>();
            static_assert_conversion<forward_proxy<const facade&&>, forward_proxy<const facade&&>, copy::disabled, move::disabled>();
            static_assert_conversion<forward_proxy<const facade&&>, view_proxy<facade>, copy::disabled, move::disabled>();
            static_assert_conversion<forward_proxy<const facade&&>, view_proxy<const facade>, copy::disabled, move::disabled>();
            static_assert_conversion<forward_proxy<const facade&&>, value_proxy<facade>, copy::disabled, move::disabled>();
            static_assert_conversion<forward_proxy<const base&&>, forward_proxy<const facade&&>, copy::disabled, move::disabled>();
        }

        using namespace proxies::tests::observable;

        SECTION("value facade to view facade")
        {
            value_proxy<facade> p1 = proxies::make_value<facade, impl>();
            view_proxy<facade> p2 = p1;

            REQUIRE(p1.ptr() == p2.ptr());
        }

        SECTION("value facade to view base")
        {
            value_proxy<facade> p1 = proxies::make_value<facade, impl>();
            view_proxy<base> p2 = p1;

            REQUIRE(p1.ptr() == p2.ptr());
        }

        SECTION("value facade to value base")
        {
            value_proxy<facade> p1 = proxies::make_value<facade, impl>();
            value_proxy<base> p2 = p1;

            REQUIRE(p2.ptr() != nullptr);
            REQUIRE(p2.ptr() != p1.ptr());
            REQUIRE(p1.as<impl>().f.copied);
        }

        SECTION("value facade to value facade copy")
        {
            value_proxy<facade> p1 = proxies::make_value<facade, impl>();
            value_proxy<facade> p2 = p1;

            REQUIRE(p2.ptr() != nullptr);
            REQUIRE(p2.ptr() != p1.ptr());
            REQUIRE(p1.as<impl>().f.copied);
        }

        SECTION("value facade to value facade move")
        {
            value_proxy<facade> p1 = proxies::make_value<facade, impl>();
            value_proxy<facade> p2 = std::move(p1);

            REQUIRE(p1.ptr() == nullptr);
            REQUIRE(p2.ptr() != nullptr);
            REQUIRE(p2.ptr() != p1.ptr());
            REQUIRE(p2.as<impl>().f.moved);
        }

        SECTION("view facade to value facade")
        {
            impl i;
            view_proxy<facade> p1 = proxies::make_view<facade>(i);
            value_proxy<facade> p2 = p1;

            REQUIRE(p2.ptr() != nullptr);
            REQUIRE(p2.ptr() != p1.ptr());
            REQUIRE(i.f.copied);
        }

        SECTION("forward facade to value facade")
        {
            impl i;
            forward_proxy<facade&&> p1 = proxies::make_forward<facade>(std::move(i));
            value_proxy<facade> p2 = p1;

            REQUIRE(p2.ptr() != nullptr);
            REQUIRE(p2.ptr() != p1.ptr());
            REQUIRE(i.f.moved);
        }
    }

    SECTION("dispatch or throw")
    {
        struct facade : proxy_facade<facade>
        {
            using dispatch_type [[maybe_unused]] = TestType;

            void act() const
            {
                constexpr auto func = []<proxies::tests::actable self_t>(const self_t& self) { self.act(); };
                proxies::dispatch_or_throw(*this, func);
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
            using dispatch_type [[maybe_unused]] = TestType;

            int act() const
            {
                constexpr auto func = []<proxies::tests::actable self_t>(const self_t& self) { self.act(); };
                return proxies::dispatch_or_default(*this, func, [] { return 0; });
            }
        };

        struct impl
        {
        };

        proxy p = proxies::make_value<facade, impl>();

        const int result = p.act();

        REQUIRE(result == 0);
    }

    SECTION("dispatch forwarding")
    {
        SECTION("r-value")
        {
            struct facade : proxy_facade<facade>
            {
                void act() &&
                {
                    constexpr auto func = []<typename self_t>(self_t&& self) { static_assert(std::is_rvalue_reference_v<self_t&&>); };
                    return proxies::dispatch(std::move(*this), func);
                }
            };

            struct impl
            {
            };

            proxy p = proxies::make_value<facade, impl>();

            std::move(p).act();
        }

        SECTION("l-value")
        {
            struct facade : proxy_facade<facade>
            {
                void act() &
                {
                    constexpr auto func = []<typename self_t>(self_t&& self) { static_assert(std::is_lvalue_reference_v<self_t>); };
                    return proxies::dispatch(*this, func);
                }
            };

            struct impl
            {
            };

            proxy p = proxies::make_value<facade, impl>();

            p.act();
        }

        SECTION("const l-value")
        {
            struct facade : proxy_facade<facade>
            {
                void act() const
                {
                    constexpr auto func = []<typename self_t>(self_t&& self) { static_assert(std::is_const_v<std::remove_reference_t<self_t>>); };
                    return proxies::dispatch(*this, func);
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
                proxies::dispatch(*this, func);
            }
        };

        struct facade_base2 : proxy_facade<facade_base2>
        {
            void func2()
            {
                constexpr auto func = [](auto& self) { self.func2(); };
                proxies::dispatch(*this, func);
            }
        };

        struct facade : proxy_facade<facade, facade_base1, facade_base2>
        {
            using dispatch_type [[maybe_unused]] = TestType;
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

        proxy p = proxies::make_value<proxies::tests::templates::facade<TestType>, proxies::tests::templates::impl>();

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
            using dispatch_type [[maybe_unused]] = proxies::tests::test_dispatch;

            void act()
            {
                constexpr auto func = [](auto& self) { self.act(); };
                proxies::dispatch(*this, func);
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
        proxy p = proxies::tests::tu::make_proxy<TestType>();

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

        proxy p = proxies::make_value<proxies::tests::threads::facade<TestType>, proxies::tests::threads::impl>();

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