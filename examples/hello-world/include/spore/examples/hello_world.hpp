#pragma once

#include "spore/proxy/proxy.hpp"

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

    struct hello_world_impl
    {
        std::string message;

        void say_message(const std::string_view name) const
        {
            std::cout << message << ", from " << name << std::endl;
        }
    };
}