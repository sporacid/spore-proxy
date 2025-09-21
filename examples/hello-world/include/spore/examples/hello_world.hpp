#pragma once

#include "spore/proxy/proxy.hpp"
#include "spore/proxy/proxy_facade.hpp"

#include <iostream>

namespace spore::examples
{
    struct message_facade : proxy_facade<message_facade>
    {
        void say_message(const std::string_view name)
        {
            constexpr auto func = [](const auto& self, const std::string_view name) {
                return self.say_message(name);
            };

            proxies::dispatch(func, *this, name);
        }
    };

    struct message_facade2 : proxy_facade<message_facade2>
    {
        void act1()
        {
            constexpr auto func = [](const auto& self) {
                return self.act1();
            };

            proxies::dispatch(func, *this);
        }
    };

    struct message_facade3 : proxy_facade<message_facade3, message_facade2, message_facade>
    {
        void act2()
        {
            constexpr auto func = [](const auto& self) {
                return self.act2();
            };

            proxies::dispatch(func, *this);
        }
    };

    struct hello_world_impl
    {
        std::string message;

        void say_message(const std::string_view name) const
        {
            std::cout << message << ", from " << name << std::endl;
        }
    };

    struct afaf_impl
    {
        void say_message(const std::string_view name) const
        {
        }

        void act1() const {}
        void act2() const {}
    };
}