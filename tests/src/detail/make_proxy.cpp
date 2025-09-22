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

    shared_proxy<facade> make_proxy()
    {
        return proxies::make_shared<facade, impl>();
    }
}