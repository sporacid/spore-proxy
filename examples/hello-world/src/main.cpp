#include "spore/examples/hello_world.hpp"

int main()
{
    using namespace spore;
    using namespace spore::examples;

    value_proxy<message_facade> proxy = proxies::make_value<message_facade, hello_world_impl>("Hello world");
    proxy.say_message("sporacid");

    return 0;
}