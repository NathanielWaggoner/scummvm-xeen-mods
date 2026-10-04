# World of Xeen mods for ScummVM

This is a fork of [ScummVM](https://github.com/scummvm/scummvm) that adds a set of
optional mods to its Might and Magic engine, for playing *Might and Magic: World of
Xeen* (Might and Magic IV and V combined). Everything lives on the `xeen-mod` branch,
with one commit per mod on top of ScummVM's development code.

You need your own copy of World of Xeen (for example from GOG). No game data is
included here.

## The mods

Each mod is a `true`/`false` setting under `[worldofxeen]` in the ScummVM
configuration file, and is off unless set.

| Setting | What it does | Keys |
|---|---|---|
| `RealTime` | Monsters move on a timer while you stand still, and combat turns time out. `RealTimeExploreMs` and `RealTimeCombatMs` set the timings. | Enter toggles |
| `AutoPlay` | The party explores and fights by itself: shoots monsters in sight, casts ready attack spells, otherwise attacks. Stops when the party is badly hurt. | P toggles |
| `LargeBackpack` | 36 backpack slots per item category instead of 9. | PgUp/PgDn or , . change page |
| `ItemStats` | Weapon damage and armor class shown beside item names without Identify. | |
| `SafeCauldrons` | Searching a vat or cauldron does no damage; whatever else it gives is unchanged. | |
| `SearchPreview` | "Who will search" prompts list what the search can give or do: stat bonuses, experience, gold, gems, items, damage. | |
| `MaxStartStats` | New characters roll 21 in every attribute. | |
| `StarterKit` | New characters get a basic weapon and armor for their class. | |
| `BulkTraining` | Training carries on through every level the character qualifies for and can afford. | |
| `TempleHealAll` | Heals everyone who needs it at a temple, at the normal price. | A at a temple |
| `LevelUpMarker` | Marks the portrait of anyone with enough experience to train. | |
| `SpaceConfirms` | Space and Enter answer yes on confirmation prompts. | |
| `TabSwitchesCharacter` | Steps between characters on screens where F1-F6 pick one. | Tab / Shift+Tab |
| `MapNotes` | Notes attached to map tiles, also marked on the minimap. | On the map: left-click adds or edits, right-click deletes, hover reads, V or N shows all. In the main view: N shows the text on the minimap |
| `MapZoom` | Three zoom levels and a movable view on the map screen. | Mouse wheel or - =, drag or arrow keys |

## Installing

The Windows installer in `installer/` has a checkbox for each mod, finds a GOG install
of the game, and copies the game data from it. See `installer/README.txt`.

To build the installer yourself: build ScummVM (below), run `installer/stage.sh` from
an MSYS2 MINGW64 shell, then compile `installer/xeen-mods.iss` with
[Inno Setup](https://jrsoftware.org/isinfo.php).

## Building

Build as for ScummVM on your platform. Only the Might and Magic engine is needed:

```
mkdir build && cd build
../configure --disable-all-engines --enable-engine=mm,xeen --enable-release
make
```

`dists/engine-data/mm.dat` on this branch has been repacked from
`devtools/create_mm/files`, because the copy in ScummVM's tree was older than the
engine expected.

## Saved games

Saves made with these mods load in standard ScummVM, but standard ScummVM drops
backpack items beyond the first nine per category and any map notes.

## Licence

ScummVM is free software under the GNU General Public License, version 3 or later;
see `COPYING`. The mods are released under the same licence. This project is not
affiliated with the ScummVM team or with the owners of Might and Magic.
