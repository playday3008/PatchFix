#include "games/batman/ak/hooks/process_event.hpp"

#include <string_view>
#include <utility>

#include "core/logger.hpp" // IWYU pragma: keep

#include "core/mem/hook.hpp"

#include "games/batman/ak/sdk.hpp"

#include "GameDefines.hpp"
#include "SDK_HEADERS/Core_classes.hpp"
#include "SDK_HEADERS/Core_structs.hpp"

namespace games::batman::ak {
    namespace {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
        Subscribers<8> s_subscribers;
        mem::MidHook   g_entry_hook;
#pragma clang diagnostic pop

        struct ProcessEventEntry {
            [[maybe_unused]] static constexpr std::string_view name = "ProcessEvent/Entry";

            [[maybe_unused]] static void operator()(mem::Registers &regs) {
                s_subscribers.dispatch(reinterpret_cast<UObject *>(regs.rcx),
                                       reinterpret_cast<UFunction *>(regs.rdx),
                                       reinterpret_cast<void *>(regs.r8));
            }
        };
    } // namespace

    auto subscribe(Handler handler) -> void {
        if (!s_subscribers.add(handler)) {
            log::get()->error(
                "Arkham Knight ProcessEventHook: subscriber limit reached, handler ignored");
        }
    }
} // namespace games::batman::ak

namespace hooks {
    auto HookTraits<games::batman::ak::ProcessEventHook>::install(const Addrs &addrs) -> bool {
        using namespace games::batman::ak;

        if (!sdk::init(addrs)) {
            log::get()->error("Arkham Knight ProcessEventHook: SDK globals did not resolve");
            return false;
        }

        // required_patterns guarantees this is set before install() runs.
        const auto process_event = addrs.process_event.value();

        auto hook = mem::make_hook<ProcessEventEntry>(process_event);
        if (!hook) {
            log::get()->error("Arkham Knight ProcessEventHook: hook failed: {}", hook.error());
            return false;
        }
        g_entry_hook = std::move(*hook);

        log::get()->info("Arkham Knight ProcessEventHook: installed at 0x{:X}", process_event);
        return true;
    }
} // namespace hooks
