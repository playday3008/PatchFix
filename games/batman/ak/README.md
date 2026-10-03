# Batman.ArkhamKnight.PatchFix

Batman: Arkham Knight plugin of [PatchFix](../../../README.md). See the main README for installation, diagnostics and building.

Steam build only for now.

## Features

- **DLC loader.** Steam build only. The Epic and GOG builds load every folder under `DLC` themselves, with no ownership check, so custom folders go directly in `DLC` there instead of `DLC/Custom`. On Steam, every folder under `DLC/Custom` (set with `[DLCLoader] CustomRoot`) is installed as a DLC bundle after the game's own DLC, with no ownership check of its own. A folder can add content, replace packages and other files of the same name, and merge its ini and int files into the game's. Folders load in name order, so zero-padded prefixes (`10_`, `20_`) set priority. It also logs the official `DLC` folders Steam did not install, and whether the name does not start with an appid or Steam does not report it owned and installed.
- **Exit freeze workaround.** Off by default. Under Wine/Proton the game sometimes freezes when quitting to desktop: shutdown waits for a background thread whose wake-up signal was lost. `[ExitFreeze] Workaround=true` makes that thread wake every 100 ms on its own, so shutdown always completes.

## Content fixes

`content/00_PatchFix` is a DLC folder with fixes that need no code: the raw `<FONT>` tags in four Most Wanted unlock requirements (all languages) and Batgirl's missing keyboard bindings for Special Combo Takedown 2 and 3 (Alt+2, Alt+3). Releases ship it as `Batman.ArkhamKnight.PatchFix-content.zip`. Copy the folder to `DLC/Custom/00_PatchFix`; on Steam that needs the DLC loader, on Epic and GOG the game loads it by itself. See its `README.txt`.

## Building

The module builds against [BmAK-UDK](https://github.com/playday3008/BmAK-UDK), an SDK generated with [CodeRed-Generator](https://github.com/playday3008/CodeRed-Generator) that CMake fetches at configure time. To build against a local checkout instead, configure with `-DFETCHCONTENT_SOURCE_DIR_BMAK-UDK=<path>`.
