#include "spore/proxy/tests/t_translation_unit.hpp"

namespace spore::proxies::tests::tu
{
    template <typename dispatch_t>
    std::size_t some_other_work(const shared_proxy<facade<dispatch_t>>& proxy)
    {
        return proxy->some_other_work();
    }

    template std::size_t some_other_work(const shared_proxy<facade<proxy_dispatch_dynamic<>>>& proxy);
    template std::size_t some_other_work(const shared_proxy<facade<proxy_dispatch_static<>>>& proxy);
}