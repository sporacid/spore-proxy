#include "catch2/catch_all.hpp"

#include "spore/proxy/proxy.hpp"
#include "spore/proxy/tests/t_semantics.hpp"

TEST_CASE("spore::proxy::semantics", "[spore::proxy][spore::proxy::semantics]")
{
    using namespace spore;

    SECTION("value semantics")
    {
        struct impl : proxy_value_semantics<proxies::tests::semantics::facade>
        {
        };

        SECTION("l-value")
        {
            impl i;
            proxies::tests::semantics::flags f;

            i.act(f);

            REQUIRE(f.lvalue);
        }

        SECTION("r-value")
        {
            impl i;
            proxies::tests::semantics::flags f;

            std::move(i).act(f);

            REQUIRE(f.rvalue);
        }

        SECTION("const l-value")
        {
            const impl i;
            proxies::tests::semantics::flags f;

            i.act(f);

            REQUIRE(f.const_lvalue);
        }

        SECTION("const r-value")
        {
            const impl i;
            proxies::tests::semantics::flags f;

            std::move(i).act(f);

            REQUIRE(f.const_rvalue);
        }
    }

    SECTION("pointer semantics")
    {
        struct impl : proxy_pointer_semantics<proxies::tests::semantics::facade>
        {
        };

        struct const_impl : proxy_pointer_semantics<const proxies::tests::semantics::facade>
        {
        };

        SECTION("l-value")
        {
            {
                impl i;
                proxies::tests::semantics::flags f;

                i->act(f);

                REQUIRE(f.lvalue);
            }

            {
                const_impl i;
                proxies::tests::semantics::flags f;

                i->act(f);

                REQUIRE(f.const_lvalue);
            }
        }

        SECTION("r-value")
        {
            {
                impl i;
                proxies::tests::semantics::flags f;

                std::move(i)->act(f);

                REQUIRE(f.lvalue);
            }

            {
                const_impl i;
                proxies::tests::semantics::flags f;

                std::move(i)->act(f);

                REQUIRE(f.const_lvalue);
            }
        }

        SECTION("const l-value")
        {
            {
                const impl i;
                proxies::tests::semantics::flags f;

                i->act(f);

                REQUIRE(f.lvalue);
            }

            {
                const const_impl i;
                proxies::tests::semantics::flags f;

                i->act(f);

                REQUIRE(f.const_lvalue);
            }
        }

        SECTION("const r-value")
        {
            {
                const impl i;
                proxies::tests::semantics::flags f;

                std::move(i)->act(f);

                REQUIRE(f.lvalue);
            }

            {
                const const_impl i;
                proxies::tests::semantics::flags f;

                std::move(i)->act(f);

                REQUIRE(f.const_lvalue);
            }
        }
    }

    SECTION("forward semantics")
    {
        struct lvalue_impl : proxy_forward_semantics<proxies::tests::semantics::facade&>
        {
        };

        struct rvalue_impl : proxy_forward_semantics<proxies::tests::semantics::facade&&>
        {
        };

        struct const_lvalue_impl : proxy_forward_semantics<const proxies::tests::semantics::facade&>
        {
        };

        struct const_rvalue_impl : proxy_forward_semantics<const proxies::tests::semantics::facade&&>
        {
        };

        SECTION("l-value")
        {
            {
                lvalue_impl i;
                proxies::tests::semantics::flags f;

                i.get().act(f);

                REQUIRE(f.lvalue);
            }

            {
                rvalue_impl i;
                proxies::tests::semantics::flags f;

                i.get().act(f);

                REQUIRE(f.rvalue);
            }

            {
                const_lvalue_impl i;
                proxies::tests::semantics::flags f;

                i.get().act(f);

                REQUIRE(f.const_lvalue);
            }

            {
                const_rvalue_impl i;
                proxies::tests::semantics::flags f;

                i.get().act(f);

                REQUIRE(f.const_rvalue);
            }
        }

        SECTION("r-value")
        {
            {
                lvalue_impl i;
                proxies::tests::semantics::flags f;

                std::move(i).get().act(f);

                REQUIRE(f.lvalue);
            }

            {
                rvalue_impl i;
                proxies::tests::semantics::flags f;

                std::move(i).get().act(f);

                REQUIRE(f.rvalue);
            }
            {
                const_lvalue_impl i;
                proxies::tests::semantics::flags f;

                std::move(i).get().act(f);

                REQUIRE(f.const_lvalue);
            }

            {
                const_rvalue_impl i;
                proxies::tests::semantics::flags f;

                std::move(i).get().act(f);

                REQUIRE(f.const_rvalue);
            }
        }

        SECTION("const l-value")
        {
            {
                const lvalue_impl i;
                proxies::tests::semantics::flags f;

                i.get().act(f);

                REQUIRE(f.lvalue);
            }

            {
                const rvalue_impl i;
                proxies::tests::semantics::flags f;

                i.get().act(f);

                REQUIRE(f.rvalue);
            }
            {
                const const_lvalue_impl i;
                proxies::tests::semantics::flags f;

                i.get().act(f);

                REQUIRE(f.const_lvalue);
            }

            {
                const const_rvalue_impl i;
                proxies::tests::semantics::flags f;

                i.get().act(f);

                REQUIRE(f.const_rvalue);
            }
        }

        SECTION("const r-value")
        {
            {
                const lvalue_impl i;
                proxies::tests::semantics::flags f;

                std::move(i).get().act(f);

                REQUIRE(f.lvalue);
            }

            {
                const rvalue_impl i;
                proxies::tests::semantics::flags f;

                std::move(i).get().act(f);

                REQUIRE(f.rvalue);
            }
            {
                const const_lvalue_impl i;
                proxies::tests::semantics::flags f;

                std::move(i).get().act(f);

                REQUIRE(f.const_lvalue);
            }

            {
                const const_rvalue_impl i;
                proxies::tests::semantics::flags f;

                std::move(i).get().act(f);

                REQUIRE(f.const_rvalue);
            }
        }
    }
}