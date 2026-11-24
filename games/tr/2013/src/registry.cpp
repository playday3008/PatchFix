#include "games/tr/2013/registry.hpp"

namespace games::tr::tr2013 {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
    auto registry() -> TR2013Registry & {
        static TR2013Registry instance;
        return instance;
    }
#pragma clang diagnostic pop
} // namespace games::tr::tr2013
