#include "games/batman/ak/registry.hpp"

namespace games::batman::ak {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
    auto registry() -> ArkhamKnightRegistry & {
        static ArkhamKnightRegistry instance;
        return instance;
    }
#pragma clang diagnostic pop
} // namespace games::batman::ak
