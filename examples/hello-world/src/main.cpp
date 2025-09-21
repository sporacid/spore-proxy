#include "spore/examples/hello_world.hpp"

struct facade : spore::proxy_facade<facade>
{
    void wtf() const
    {
        constexpr auto func = [](const auto& self) {
            return self.wtf();
        };

        spore::proxies::dispatch(func, *this);
    }
};

struct impl
{
    void wtf() const
    {
        std::cout << "wtf"<< std::endl;
    }
};

int main()
{
    using namespace spore;
    using namespace spore::examples;

    unique_proxy<message_facade> proxy = proxies::make_unique<message_facade, hello_world_impl>("Hello world");
    proxy.say_message("sporacid");

    // unique_proxy<facade> p1 = proxies::make_unique<facade, impl>();
    // p1.wtf();
//
    // constexpr auto l1 = proxies::detail::type_lists::get<proxies::detail::base_tag<facade>>();
    // constexpr auto l2 = proxies::detail::type_lists::get<proxies::detail::dispatch_tag<facade>>();
    // constexpr auto l3 = proxies::detail::type_lists::get<proxies::detail::value_tag<facade>>();
//
    // std::cout << typeid(l1).name() << std::endl;
    // std::cout << typeid(l2).name() << std::endl;
    // std::cout << typeid(l3).name() << std::endl;

    unique_proxy<message_facade3> p1 = proxies::make_unique<message_facade3, afaf_impl>();
//
    // std::cout << typeid(proxies::detail::type_lists::get<proxies::detail::base_tag<message_facade>>()).name() << std::endl;
    // std::cout << typeid(proxies::detail::type_lists::get<proxies::detail::dispatch_tag<message_facade>>()).name() << std::endl;
    // std::cout << typeid(proxies::detail::type_lists::get<proxies::detail::value_tag<message_facade>>()).name() << std::endl;
//
    // std::cout << typeid(proxies::detail::type_lists::get<proxies::detail::base_tag<message_facade3>>()).name() << std::endl;
    // std::cout << typeid(proxies::detail::type_lists::get<proxies::detail::dispatch_tag<message_facade3>>()).name() << std::endl;
    // std::cout << typeid(proxies::detail::type_lists::get<proxies::detail::value_tag<message_facade3>>()).name() << std::endl;

    p1.act2();
    p1.act1();
    p1.say_message("");
    return 0;
}