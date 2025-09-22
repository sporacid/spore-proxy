#include "spore/proxy/tests/t_translation_unit.hpp"

namespace spore::proxies::tests
{
    struct translation_unit_impl
    {
        template <std::size_t value_v>
        std::size_t act() const
        {
            return value_v;
        }
    };

    shared_proxy<translation_unit_facade> make_proxy()
    {
        return proxies::make_shared<translation_unit_facade, translation_unit_impl>();
    }
}