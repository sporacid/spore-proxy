#include "spore/proxy/tests/t_translation_unit.hpp"

namespace spore::proxies::tests
{
    struct translation_unit_impl
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

    shared_proxy<translation_unit_facade> make_proxy()
    {
        return proxies::make_shared<translation_unit_facade, translation_unit_impl>();
    }
}