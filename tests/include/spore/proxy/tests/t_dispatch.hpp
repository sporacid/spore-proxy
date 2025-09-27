#pragma once

namespace spore::proxies::tests
{
    template <typename value_t>
    concept actable = requires(const value_t& value) {
        { value.act() };
    };

    struct test_dispatch
    {
        static inline bool get_called = false;
        static inline bool set_called = false;

        template <typename mapping_t>
        struct dispatch_null;

        template <typename facade_t, typename func_t, typename self_t, typename return_t, typename... args_t>
        struct dispatch_null<proxies::detail::dispatch_mapping<facade_t, func_t, self_t, return_t(args_t...)>>
        {
            using void_type = proxies::detail::dispatch_mapping<facade_t, func_t, self_t, return_t(args_t...)>::void_type;

            SPORE_PROXY_FORCE_INLINE static constexpr return_t dispatch(void_type*, args_t&&...) SPORE_PROXY_THROW_SPEC
            {
                if constexpr (not std::is_void_v<return_t>)
                {
                    return return_t {};
                }
            }
        };

        template <typename tag_t, typename func_t>
        SPORE_PROXY_FORCE_INLINE static void call_once(const func_t&)
        {
            func_t {}();
        }

        template <typename mapping_t>
        [[nodiscard]] SPORE_PROXY_FORCE_INLINE static constexpr proxies::detail::dispatch_type<mapping_t> get_dispatch(const std::uint32_t type_index) noexcept
        {
            get_called = true;
            return &dispatch_null<mapping_t>::dispatch;
        }

        template <typename mapping_t>
        SPORE_PROXY_FORCE_INLINE static void set_dispatch(const std::uint32_t type_index, const proxies::detail::dispatch_type<mapping_t> dispatcher) noexcept
        {
            set_called = true;
        }
    };
}