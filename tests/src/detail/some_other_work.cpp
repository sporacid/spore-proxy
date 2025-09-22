#include "spore/proxy/tests/t_translation_unit.hpp"

namespace spore::proxies::tests::tu
{
    std::size_t some_other_work(const shared_proxy<facade>& proxy)
    {
        return proxy.some_other_work();
    }
}