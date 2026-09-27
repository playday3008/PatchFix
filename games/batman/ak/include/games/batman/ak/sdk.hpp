#pragma once

#include "games/batman/ak/game_data.hpp"

namespace games::batman::ak::sdk {
    using Addrs = game_data<ArkhamKnight>::ResolvedAddresses;

    // Points the SDK's GObjects and GNames at the game's globals and stores the
    // game allocator. Call once, before any SDK object access.
    auto init(const Addrs &addrs) -> bool;
} // namespace games::batman::ak::sdk
