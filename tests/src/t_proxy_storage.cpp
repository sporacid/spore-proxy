#include "catch2/catch_all.hpp"

#include "spore/proxy/proxy.hpp"
#include "spore/proxy/tests/t_observable.hpp"

TEST_CASE("spore::proxy::storage", "[spore::proxy][spore::proxy::storage]")
{
    using namespace spore;
    using namespace spore::proxies::tests::observable;

    SECTION("shared storage")
    {
        using proxy_storage_shared_t = proxy_storage_shared<std::uint32_t>;

        SECTION("in-place construction")
        {
            proxy_storage_shared_t s {std::in_place_type<impl>};

            REQUIRE(s.counter() == 1);
            REQUIRE(s.ptr() != nullptr);
        }

        SECTION("copy")
        {
            bool copied = false;
            bool moved = false;

            proxy_storage_shared_t s1 {std::in_place_type<impl>, flags {.copied = copied, .moved = moved}};
            proxy_storage_shared_t s2 = s1;

            REQUIRE(s1.ptr() == s2.ptr());
            REQUIRE(s1.counter() == 2);
            REQUIRE(s2.counter() == 2);
            REQUIRE(std::addressof(s1.counter()) == std::addressof(s2.counter()));

            void* ptr = s1.ptr();
            s2.reset();

            REQUIRE(s1.counter() == 1);
            REQUIRE(s1.ptr() == ptr);

            REQUIRE_FALSE(copied);
            REQUIRE_FALSE(moved);
        }

        SECTION("move")
        {
            bool copied = false;
            bool moved = false;

            proxy_storage_shared_t s1 {std::in_place_type<impl>, flags {.copied = copied, .moved = moved}};

            void* ptr = s1.ptr();

            proxy_storage_shared_t s2 = std::move(s1);

            REQUIRE(s1.ptr() == nullptr);
            REQUIRE(s2.counter() == 1);
            REQUIRE(s2.ptr() == ptr);

            REQUIRE_FALSE(copied);
            REQUIRE_FALSE(moved);
        }

        SECTION("destruction")
        {
            bool destroyed = false;

            proxy_storage_shared_t s {std::in_place_type<impl>, flags {.destroyed = destroyed}};

            s.reset();

            REQUIRE(s.ptr() == nullptr);
            REQUIRE(destroyed);
        }
    }

    SECTION("unique storage")
    {
        static_assert(not std::is_copy_constructible_v<proxy_storage_unique>);
        static_assert(not std::is_copy_assignable_v<proxy_storage_unique>);

        SECTION("in-place construction")
        {
            proxy_storage_unique s {std::in_place_type<impl>};

            REQUIRE(s.ptr() != nullptr);
        }

        SECTION("move")
        {
            bool copied = false;
            bool moved = false;

            proxy_storage_unique s1 {std::in_place_type<impl>, flags {.copied = copied, .moved = moved}};

            void* ptr = s1.ptr();

            proxy_storage_unique s2 = std::move(s1);

            REQUIRE(s1.ptr() == nullptr);
            REQUIRE(s2.ptr() == ptr);

            REQUIRE_FALSE(moved);
            REQUIRE_FALSE(copied);
        }

        SECTION("destruction")
        {
            bool destroyed = false;

            proxy_storage_unique s {std::in_place_type<impl>, flags {.destroyed = destroyed}};

            s.reset();

            REQUIRE(s.ptr() == nullptr);
            REQUIRE(destroyed);
        }
    }

    SECTION("value storage")
    {
        SECTION("in-place construction")
        {
            proxy_storage_value s {std::in_place_type<impl>};

            REQUIRE(s.ptr() != nullptr);
        }

        SECTION("copy")
        {
            bool copied = false;
            bool moved = false;

            proxy_storage_value s1 {std::in_place_type<impl>, flags {.copied = copied, .moved = moved}};
            proxy_storage_value s2 = s1;

            REQUIRE(s1.ptr() != s2.ptr());
            REQUIRE(copied);
            REQUIRE_FALSE(moved);
        }

        SECTION("move")
        {
            bool copied = false;
            bool moved = false;

            proxy_storage_value s1 {std::in_place_type<impl>, flags {.copied = copied, .moved = moved}};
            proxy_storage_value s2 = std::move(s1);

            REQUIRE(s1.ptr() != nullptr);
            REQUIRE(s2.ptr() != nullptr);

            REQUIRE(moved);
            REQUIRE_FALSE(copied);
        }

        SECTION("destruction")
        {
            bool destroyed = false;

            proxy_storage_value s {std::in_place_type<impl>, flags {.destroyed = destroyed}};

            s.reset();

            REQUIRE(s.ptr() == nullptr);
            REQUIRE(destroyed);
        }
    }

    SECTION("sbo storage")
    {
        using proxy_storage_sbo_t = proxy_storage_sbo<sizeof(impl), alignof(impl)>;

        SECTION("in-place construction")
        {
            proxy_storage_sbo_t s {std::in_place_type<impl>};

            REQUIRE(s.ptr() != nullptr);
        }

        SECTION("copy")
        {
            bool copied = false;
            bool moved = false;

            proxy_storage_sbo_t s1 {std::in_place_type<impl>, flags {.copied = copied, .moved = moved}};
            proxy_storage_sbo_t s2 = s1;

            REQUIRE(s1.ptr() != s2.ptr());
            REQUIRE(copied);
            REQUIRE_FALSE(moved);
        }

        SECTION("move")
        {
            bool copied = false;
            bool moved = false;

            proxy_storage_sbo_t s1 {std::in_place_type<impl>, flags {.copied = copied, .moved = moved}};
            proxy_storage_sbo_t s2 = std::move(s1);

            REQUIRE(s1.ptr() != nullptr);
            REQUIRE(s2.ptr() != nullptr);

            REQUIRE(moved);
            REQUIRE_FALSE(copied);
        }

        SECTION("destruction")
        {
            bool destroyed = false;

            proxy_storage_sbo_t s {std::in_place_type<impl>, flags {.destroyed = destroyed}};

            s.reset();

            REQUIRE(s.ptr() == nullptr);
            REQUIRE(destroyed);
        }
    }

    SECTION("inline storage")
    {
        using proxy_storage_inline_t = proxy_storage_inline<impl>;

        SECTION("in-place construction")
        {
            proxy_storage_inline_t s {std::in_place_type<impl>};

            REQUIRE(s.ptr() != nullptr);
        }

        SECTION("copy")
        {
            bool copied = false;
            bool moved = false;

            proxy_storage_inline_t s1 {std::in_place_type<impl>, flags {.copied = copied, .moved = moved}};
            proxy_storage_inline_t s2 = s1;

            REQUIRE(s1.ptr() != s2.ptr());
            REQUIRE(copied);
            REQUIRE_FALSE(moved);
        }

        SECTION("move")
        {
            bool copied = false;
            bool moved = false;

            proxy_storage_inline_t s1 {std::in_place_type<impl>, flags {.copied = copied, .moved = moved}};
            proxy_storage_inline_t s2 = std::move(s1);

            REQUIRE(s1.ptr() != nullptr);
            REQUIRE(s2.ptr() != nullptr);

            REQUIRE(moved);
            REQUIRE_FALSE(copied);
        }

        SECTION("destruction")
        {
            bool destroyed = false;

            proxy_storage_inline_t s {std::in_place_type<impl>, flags {.destroyed = destroyed}};

            s.reset();

            REQUIRE(s.ptr() == nullptr);
            REQUIRE(destroyed);
        }
    }

    SECTION("non-owning storage")
    {
        SECTION("in-place construction")
        {
            impl i;

            proxy_storage_non_owning s {std::in_place_type<impl>, i};

            REQUIRE(s.ptr() != nullptr);
            REQUIRE(s.ptr() == std::addressof(i));
        }

        SECTION("copy")
        {
            bool copied = false;
            bool moved = false;

            impl i {flags {.copied = copied, .moved = moved}};

            proxy_storage_non_owning s1 {std::in_place_type<impl>, i};
            proxy_storage_non_owning s2 = s1;

            REQUIRE(s1.ptr() == s2.ptr());
            REQUIRE(s2.ptr() == std::addressof(i));

            REQUIRE_FALSE(copied);
            REQUIRE_FALSE(moved);
        }

        SECTION("move")
        {
            bool copied = false;
            bool moved = false;

            impl i {flags {.copied = copied, .moved = moved}};

            proxy_storage_non_owning s1 {std::in_place_type<impl>, i};
            proxy_storage_non_owning s2 = std::move(s1);

            REQUIRE(s1.ptr() == s2.ptr());
            REQUIRE(s2.ptr() == std::addressof(i));

            REQUIRE_FALSE(moved);
            REQUIRE_FALSE(copied);
        }

        SECTION("destruction")
        {
            bool destroyed = false;

            impl i {flags {.destroyed = destroyed}};

            proxy_storage_non_owning s {std::in_place_type<impl>, i};

            s.reset();

            REQUIRE(s.ptr() == nullptr);
            REQUIRE_FALSE(destroyed);
        }
    }
}