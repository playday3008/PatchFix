PatchFix content fixes for Batman: Arkham Knight
================================================

Game data fixes that need no changes to the game executable.

Install
-------
Copy this folder to:

    <game folder>\DLC\Custom\00_PatchFix

Steam: needs Batman.ArkhamKnight.PatchFix.asi with DLCLoader enabled
(the default), which loads every folder under DLC\Custom.
Epic and GOG: the game loads every folder under DLC by itself, no plugin
needed for this folder.

Folders under DLC\Custom load in name order and later folders win, so
other mods placed next to this one override it.

Fixes
-----
- Most Wanted: the unlock requirement of the Killer Croc, Mr. Freeze,
  Mad Hatter and Ra's al Ghul missions showed raw <FONT COLOR=...> tags.
  The tags are removed in all 11 languages; the text is otherwise
  unchanged and is no longer highlighted.
- Batgirl (A Matter of Family): Special Combo Takedown 2 and 3 had no
  keyboard binding. They are added on Alt+2 and Alt+3, the same keys
  Batman uses.
