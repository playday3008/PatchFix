# Batman.ArkhamKnight.PatchFix

Batman: Arkham Knight plugin of [PatchFix](../../../README.md). See the main README for installation, diagnostics and building.

Steam build only for now.

## Features

- **DLC loader.** Steam build only. The Epic and GOG builds load every folder under `DLC` themselves, with no ownership check, so custom folders go directly in `DLC` there instead of `DLC/Custom`. On Steam, every folder under `DLC/Custom` (set with `[DLCLoader] CustomRoot`) is installed as a DLC bundle after the game's own DLC, with no ownership check of its own. A folder can add content, replace packages and other files of the same name, and merge its ini and int files into the game's. Folders load in name order, so zero-padded prefixes (`10_`, `20_`) set priority. It also logs the official `DLC` folders Steam did not install, and whether the name does not start with an appid or Steam does not report it owned and installed.

## Building

The module builds against [BmAK-UDK](https://github.com/playday3008/BmAK-UDK), an SDK generated with [CodeRed-Generator](https://github.com/playday3008/CodeRed-Generator) that CMake fetches at configure time. To build against a local checkout instead, configure with `-DFETCHCONTENT_SOURCE_DIR_BMAK-UDK=<path>`.
