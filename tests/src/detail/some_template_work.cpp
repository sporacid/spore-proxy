#include "spore/proxy/tests/t_translation_unit.hpp"

namespace spore::proxies::tests::tu
{
    template <typename dispatch_t>
    std::size_t some_template_work(const shared_proxy<facade<dispatch_t>>& proxy)
    {
        return proxy->template some_template_work<1>() + proxy->template some_template_work<2>();
    }

    template std::size_t some_template_work(const shared_proxy<facade<proxy_dispatch_dynamic<>>>& proxy);
    template std::size_t some_template_work(const shared_proxy<facade<proxy_dispatch_static<>>>& proxy);
}