# TR.2013.PatchFix

Tomb Raider (2013) plugin of [PatchFix](../../../README.md). See the main README for diagnostics and building.

Work in progress: the plugin loads and logs, but no fixes ship yet. The target is the HUD elements that end up partly off-screen on the current PC build (Patch 838, Steam build 9573671), which build `743.0` does not have.

## Installation

The game is 32-bit, so it needs the x86 build of an ASI loader. Place `TR.2013.PatchFix.asi` and `TR.2013.PatchFix.ini` next to `TombRaider.exe` together with [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases) (Win32) named `version.dll`. Releases are tagged `tr-2013/vX.Y.Z`.

On Linux / Proton, add `WINEDLLOVERRIDES="version=n,b" %command%` to the launch options.
