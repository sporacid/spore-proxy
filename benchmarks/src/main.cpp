#include "spore/proxy/proxy.hpp"

#include "proxy/proxy.h"

#include <chrono>
#include <format>
#include <functional>
#include <iostream>

namespace spore::benchmarks
{
    struct result
    {
        std::string name;
        std::double_t time;
    };

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
        const auto then = std::chrono::high_resolution_clock::now();

        func();

        const auto now = std::chrono::high_resolution_clock::now();
        const std::chrono::duration<std::double_t> duration = now - then;

        return result {
            .name = std::string {name},
            .time = duration.count(),
        };
    }

    std::size_t do_work(const std::size_t size)
    {
        std::size_t result = 0;

        for (std::size_t index = 0; index < size; index++)
        {
            result += index;
        }

        return result;
    }

    namespace virtual_
    {
        struct facade
        {
            virtual ~facade() = default;
            virtual std::size_t work(std::size_t) const = 0;
        };

        struct impl : facade
        {
            std::size_t work(const std::size_t size) const override
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
        PRO_DEF_MEM_DISPATCH(mem_work, work);

        // clang-format off
        struct facade :
            pro::facade_builder
               ::add_convention<mem_work, std::size_t(std::size_t) const>
               ::build {};
        // clang-format on

        struct impl
        {
            std::size_t work(const std::size_t size) const
            {
                return do_work(size);
            }
        };
    }

    namespace spore
    {
        struct facade : proxy_facade<facade>
        {
            std::size_t work(const std::size_t size) const
            {
                constexpr auto func = [](auto& self, const std::size_t size) { return self.work(size); };
                return proxies::dispatch<int>(func, *this, size);
            }
        };

        struct impl
        {
            std::size_t work(const std::size_t size) const
            {
                return do_work(size);
            }
        };
    }
}

int main()
{
    using namespace spore;

    constexpr std::size_t warm_iterations = 1000;
    constexpr std::size_t work_iterations = 100000000;
    constexpr std::size_t work_size = 100;

    std::vector<benchmarks::result> results;

    {
        std::unique_ptr<benchmarks::virtual_::facade> facade = std::make_unique<benchmarks::virtual_::impl>();

        for (std::size_t index = 0; index < warm_iterations; ++index)
        {
            std::ignore = facade->work(work_size);
        }

        results.emplace_back() = benchmarks::run_benchmark("virtual", [&] {
            for (std::size_t index = 0; index < work_iterations; ++index)
            {
                std::ignore = facade->work(work_size);
            }
        });
    }

    {
        std::unique_ptr<benchmarks::functional::facade> facade = std::make_unique<benchmarks::functional::impl>();

        for (std::size_t index = 0; index < warm_iterations; ++index)
        {
            std::ignore = facade->work(work_size);
        }

        results.emplace_back() = benchmarks::run_benchmark("functional", [&] {
            for (std::size_t index = 0; index < work_iterations; ++index)
            {
                std::ignore = facade->work(work_size);
            }
        });
    }

    {
        pro::proxy<benchmarks::microsoft::facade> facade = std::make_unique<benchmarks::microsoft::impl>();

        for (std::size_t index = 0; index < warm_iterations; ++index)
        {
            std::ignore = facade->work(work_size);
        }

        results.emplace_back() = benchmarks::run_benchmark("microsoft", [&] {
            for (std::size_t index = 0; index < work_iterations; ++index)
            {
                std::ignore = facade->work(work_size);
            }
        });
    }

    {
        unique_proxy<benchmarks::spore::facade> facade = proxies::make_unique<benchmarks::spore::facade, benchmarks::spore::impl>();

        for (std::size_t index = 0; index < warm_iterations; ++index)
        {
            std::ignore = facade.work(work_size);
        }

        results.emplace_back() = benchmarks::run_benchmark("spore", [&] {
            for (std::size_t index = 0; index < work_iterations; ++index)
            {
                std::ignore = facade.work(work_size);
            }
        });
    }

    benchmarks::output_results(results);
    return 0;
}