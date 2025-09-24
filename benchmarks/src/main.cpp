#include "spore/proxy/proxy.hpp"

#include "avask/some.hpp"
#include "dyno.hpp"
#include "proxy/proxy.h"

#include <chrono>
#include <format>
#include <functional>
#include <iostream>
#include <ranges>

namespace spore::benchmarks
{
    struct result
    {
        std::string name;
        std::double_t time;
    };

    template <typename value_t>
    SPORE_PROXY_FORCE_INLINE void do_not_optimize(value_t& value)
    {
#if defined(__clang__)
        asm volatile("" : "+r,m"(value) : : "memory");
#else
        asm volatile("" : "+m,r"(value) : : "memory");
#endif
    }

    void output_results(const std::span<const result> results)
    {
        std::cout << std::format("| {0:-^15} | {0:-^10} |", "") << std::endl;
        std::cout << std::format("| {:<15} | {:>10} |", "Name", "Seconds") << std::endl;
        std::cout << std::format("| {0:-^15} | {0:-^10} |", "") << std::endl;

        for (const result& result : results)
        {
            std::cout << std::format("| {:<15} | {:>10.4f} |", result.name, result.time) << std::endl;
        }

        std::cout << std::format("| {0:-^15} | {0:-^10} |", "") << std::endl;
    }

    template <typename func_t>
    result run_benchmark(const std::string_view name, func_t&& func)
    {
        const auto then = std::chrono::steady_clock::now();

        func();

        const auto now = std::chrono::steady_clock::now();
        const std::chrono::duration<std::double_t> duration = now - then;

        return result {
            .name = std::string {name},
            .time = duration.count(),
        };
    }

    SPORE_PROXY_FORCE_INLINE std::size_t do_work(const std::size_t size) noexcept
    {
        std::size_t result = 0;

        for (std::size_t index = 0; index < size; index++)
        {
            result += index;
        }

        return result;
    }

    namespace non_virtual
    {
        struct facade
        {
            std::size_t work(const std::size_t size) const noexcept
            {
                return do_work(size);
            }
        };
    }

    namespace crtp
    {
        template <typename impl_t>
        struct facade
        {
            std::size_t work(const std::size_t size) const noexcept
            {
                return static_cast<const impl_t&>(*this).work_impl(size);
            }
        };

        struct impl : facade<impl>
        {
            std::size_t work_impl(const std::size_t size) const noexcept
            {
                return do_work(size);
            }
        };
    }

    namespace virtual_
    {
        struct facade
        {
            virtual ~facade() = default;
            virtual std::size_t work(std::size_t) const noexcept = 0;
        };

        struct impl : facade
        {
            std::size_t work(const std::size_t size) const noexcept override
            {
                return do_work(size);
            }
        };
    }

    namespace functional
    {
        struct facade
        {
            std::function<std::size_t(std::size_t)> work;
        };

        struct impl : facade
        {
            impl()
            {
                work = &do_work;
            }
        };
    }

    namespace microsoft
    {

#if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wc++23-extensions"
#endif
        PRO_DEF_MEM_DISPATCH(mem_work, work);
#if defined(__clang__)
#    pragma clang diagnostic pop
#endif

        // clang-format off
        struct facade :
            pro::facade_builder
               ::add_convention<mem_work, std::size_t(std::size_t) const noexcept>
               ::build {};
        // clang-format on

        struct impl
        {
            std::size_t work(const std::size_t size) const noexcept
            {
                return do_work(size);
            }
        };
    }

    namespace dyno_
    {
        DYNO_INTERFACE(facade,
            (work, std::size_t(std::size_t) const));

        struct impl
        {
            std::size_t work(const std::size_t size) const noexcept
            {
                return do_work(size);
            }
        };
    }

    namespace avask
    {
        struct facade : vx::trait
        {
            virtual std::size_t work(std::size_t) const noexcept = 0;
        };

        struct impl
        {
            std::size_t work(const std::size_t size) const noexcept
            {
                return do_work(size);
            }
        };
    }

    namespace spore
    {
        struct facade : proxy_facade<facade>
        {
            std::size_t work(const std::size_t size) const noexcept
            {
                constexpr auto func = [](auto& self, const std::size_t size) { return self.work(size); };
                return proxies::dispatch<std::size_t>(func, *this, size);
            }
        };

        struct impl
        {
            std::size_t work(const std::size_t size) const noexcept
            {
                return do_work(size);
            }
        };
    }
}

template <typename value_t>
struct vx::impl<spore::benchmarks::avask::facade, value_t> final : impl_for<spore::benchmarks::avask::facade, value_t>
{
    using impl_for<spore::benchmarks::avask::facade, spore::benchmarks::avask::impl>::impl_for;
    using impl_for<spore::benchmarks::avask::facade, spore::benchmarks::avask::impl>::self;

    std::size_t work(const std::size_t size) const noexcept
    {
        return vx::poly {this}->work(size);
    }
};

int main()
{
    using namespace spore;

    constexpr std::size_t warm_iterations = 100;
    constexpr std::size_t work_iterations = 1000000000;
    constexpr std::size_t work_size = 100;

    std::vector<benchmarks::result> results;

    const auto benchmark = [&]<auto work_v>(const auto name, const auto& facade) {
        for (std::size_t index = 0; index < warm_iterations; ++index)
        {
            std::size_t result = work_v(facade, work_size);
            benchmarks::do_not_optimize(result);
        }

        results.emplace_back() = benchmarks::run_benchmark(name, [&] {
            for (std::size_t index = 0; index < work_iterations; ++index)
            {
                std::size_t result = work_v(facade, work_size);
                benchmarks::do_not_optimize(result);
            }
        });
    };

    constexpr auto work_pointer = [](const auto& facade, const auto work_size) {
        return facade->work(work_size);
    };

    constexpr auto work_value = [](const auto& facade, const auto work_size) {
        return facade.work(work_size);
    };

    {
        std::unique_ptr<benchmarks::virtual_::facade> facade = std::make_unique<benchmarks::virtual_::impl>();
        benchmark.template operator()<work_pointer>("virtual", facade);
    }

    {
        std::unique_ptr<benchmarks::non_virtual::facade> facade = std::make_unique<benchmarks::non_virtual::facade>();
        benchmark.template operator()<work_pointer>("non-virtual", facade);
    }

    {
        std::unique_ptr<benchmarks::crtp::facade<benchmarks::crtp::impl>> facade = std::make_unique<benchmarks::crtp::facade<benchmarks::crtp::impl>>();
        benchmark.template operator()<work_pointer>("crtp", facade);
    }

    {
        std::unique_ptr<benchmarks::functional::facade> facade = std::make_unique<benchmarks::functional::impl>();
        benchmark.template operator()<work_pointer>("functional", facade);
    }

    {
        pro::proxy<benchmarks::microsoft::facade> facade = std::make_unique<benchmarks::microsoft::impl>();
        benchmark.template operator()<work_pointer>("microsoft", facade);
    }

    {
        vx::some<benchmarks::avask::facade> facade = benchmarks::avask::impl {};
        benchmark.template operator()<work_pointer>("avask", facade);
    }

    {
        benchmarks::dyno_::facade facade {benchmarks::dyno_::impl {}};
        benchmark.template operator()<work_value>("dyno", facade);
    }

    {
        unique_proxy<benchmarks::spore::facade> facade = proxies::make_unique(benchmarks::spore::impl {});
        benchmark.template operator()<work_value>("spore (unique)", facade);
    }

    {
        shared_proxy<benchmarks::spore::facade> facade = proxies::make_shared(benchmarks::spore::impl {});
        benchmark.template operator()<work_value>("spore (shared)", facade);
    }

    {
        value_proxy<benchmarks::spore::facade> facade = proxies::make_value(benchmarks::spore::impl {});
        benchmark.template operator()<work_value>("spore (value)", facade);
    }

    {
        inline_proxy<benchmarks::spore::facade, benchmarks::spore::impl> facade = proxies::make_inline(benchmarks::spore::impl {});
        benchmark.template operator()<work_value>("spore (inline)", facade);
    }

    {
        const benchmarks::spore::impl impl;
        proxy_view<benchmarks::spore::facade> facade = proxies::make_view<benchmarks::spore::facade>(impl);
        benchmark.template operator()<work_value>("spore (view)", facade);
    }

    benchmarks::output_results(results);
    return 0;
}