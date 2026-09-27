#include "games/batman/ak/hooks/process_event.hpp"

#include "core/logger.hpp" // IWYU pragma: keep

#include "games/batman/ak/sdk.hpp"

namespace hooks {
    auto HookTraits<games::batman::ak::ProcessEventHook>::install(const Addrs &addrs) -> bool {
        if (!games::batman::ak::sdk::init(addrs)) {
            log::get()->error("Arkham Knight ProcessEventHook: SDK globals did not resolve");
            return false;
        }
        return true;
    }
} // namespace hooks
