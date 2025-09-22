#include "spore/proxy/tests/t_translation_unit.hpp"

namespace spore::proxies::tests
{
    std::size_t some_work(const shared_proxy<translation_unit_facade>& proxy)
    {
        return proxy.act<0>();
    }
}