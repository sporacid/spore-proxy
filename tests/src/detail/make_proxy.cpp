#include "spore/proxy/tests/t_translation_unit.hpp"

namespace spore::proxies::tests::tu
{
    struct impl
    {
        std::size_t some_work() const
        {
            return 0;
        }

        std::size_t some_other_work() const
        {
            return 1;
        }
    };

    template <typename dispatch_t>
    shared_proxy<facade<dispatch_t>> make_proxy()
    {
        return proxies::make_shared<facade<dispatch_t>, impl>();
    }

    template shared_proxy<facade<proxy_dispatch_dynamic<>>> make_proxy();
    template shared_proxy<facade<proxy_dispatch_static<>>> make_proxy();
}