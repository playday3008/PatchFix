#pragma once

#include "core/hooks/registry/hook_traits.hpp"

namespace hooks {
    template<typename... Ts>
    struct dep_list {};

    template<ValidHookTraits... Ts>
    struct hook_list {};
} // namespace hooks
