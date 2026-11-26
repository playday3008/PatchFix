# TR.2013.PatchFix

Tomb Raider (2013) plugin of [PatchFix](../../../README.md). See the main README for diagnostics and building.

## Features

- **HUD relayout** - fixes HUD elements drawn partly off-screen or stuck in the top-left corner on the current PC build (Patch 838, Steam build 9573671): interaction prompts, collectible counters, notifications, the salvage counter. Since Patch 829 each HUD movie applies its screen layout only on its first two frames, so anything created before the layout settles keeps a wrong placement. The fix applies it every frame again, as build `743.0` does.
- **Camera shake toggle** - `[Camera] DisableShake=true` removes camera shake: the wobble and the brief FOV pulse on impacts, in gameplay and in-engine cutscenes. Off by default. The hex edit on PCGamingWiki only covers the first of these four sites, and only on build `743.0`.
- **Ultrawide cutscenes** - in-engine cutscenes stay Hor+ in exclusive fullscreen on displays wider than 16:9. The game computes their field of view from the monitor's aspect in exclusive fullscreen but from 16:9 in a window, so they were cropped at the top and bottom unless you played windowed. `[Camera] ClampCinematicAspect=false` restores stock behavior.
- **Field of view** - `[Camera] FovMultiplier` scales the field of view of the gameplay cameras: exploring, climbing, aiming and scripted gameplay cameras. Cutscenes keep their framing. It scales the value each camera computes every frame instead of editing camera data, which is where the community FOV tool on PCGamingWiki stops Lara climbing some walls and crawling through gaps. `1.0` (stock) by default.
- **Frame limiter** - `[FPS] FrameCap` caps the frame rate; `0` (off) by default. The game has no cap of its own besides VSync, and at high frame rates the QTE in the Endurance boss fight never shows its second prompt. `FrameCap=60` replaces the external 60 FPS cap PCGamingWiki suggests.
- **Voice language is saved** (non-Steam; checked on Epic, GOG untested) - the voice language chosen in the options menu is written to the `VOLanguage` setting the game reads at startup and when Display Options are applied, so it no longer reverts, and the options menu shows the voice language actually in use instead of a different one. This replaces the registry edit on PCGamingWiki. The Steam build takes its voice language from the Steam client language and has no such setting, so there the hook is skipped and its patterns are logged as not found.
- **Saves work across Steam and non-Steam versions** (checked on Epic, GOG untested) - save files copied between the Steam and non-Steam versions load. The game state is the same in both; the Steam build additionally stores each save's slot info (date, progress) after the game state and reads the load-menu entry from there, while non-Steam builds read it from the start of the game state. Non-Steam builds rejected Steam saves as corrupt because of those 40 extra bytes; such a save is now decompressed again with room for them and accepted only if that passes zlib's checksum. The Steam build showed non-Steam saves with whatever was in memory as their date and progress; the slot info is now filled from the start of the game state. The two usually match, so a moved save can show a slightly different date or progress than on its original version.

## Installation

The game is 32-bit, so it needs the x86 build of an ASI loader. Place `TR.2013.PatchFix.asi` and `TR.2013.PatchFix.ini` next to `TombRaider.exe` together with [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases) (Win32) named `version.dll`. Releases are tagged `tr-2013/vX.Y.Z`.

On Linux / Proton, add `WINEDLLOVERRIDES="version=n,b" %command%` to the launch options.
