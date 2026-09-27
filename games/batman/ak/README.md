# Batman.ArkhamKnight.PatchFix

Batman: Arkham Knight plugin of [PatchFix](../../../README.md). See the main README for installation, diagnostics and building.

Steam build only for now.

## Features

- **DLC loader.** Every folder under `DLC/Custom` (set with `[DLCLoader] CustomRoot`) is installed as a DLC bundle after the game's own DLC, with no Steam ownership check. A folder can add content or replace base game and DLC files of the same name; folders load in name order, so a later folder wins.

## Building

The module builds against [BmAK-UDK](https://github.com/playday3008/BmAK-UDK), an SDK generated with [CodeRed-Generator](https://github.com/playday3008/CodeRed-Generator) that CMake fetches at configure time. To build against a local checkout instead, configure with `-DFETCHCONTENT_SOURCE_DIR_BMAK-UDK=<path>`.
