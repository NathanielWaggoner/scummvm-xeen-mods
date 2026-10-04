World of Xeen - Modded (ScummVM)
================================

This is a modified build of ScummVM for playing Might and Magic: World of Xeen
(Might and Magic IV and V combined) with a set of optional mods.

You need your own copy of World of Xeen, for example from GOG. The installer
asks for its folder and copies the game data from it. No game data is included.


The mods
--------

Each mod can be switched on or off. Run the installer again to change your
choices, or edit the true/false settings under [worldofxeen] in
scummvm-mod.ini in the install folder.

Real-time mode with pause (RealTime)
    Monsters move every 2 seconds while you stand still, and a character who
    takes longer than 3 seconds in combat loses their go. Enter switches
    between real-time and the original turn-based play. The timings are
    RealTimeExploreMs and RealTimeCombatMs in the settings file.

Larger backpack (LargeBackpack)
    36 slots per item category instead of 9. PgUp/PgDn (or , and .) change
    page in the items screen.

Bulk training (BulkTraining)
    One press of Train goes through every level the character qualifies for
    and the party can afford.

Temple heal-all (TempleHealAll)
    Press A at a temple to heal everyone who needs it, at the normal price.

Level-up marker (LevelUpMarker)
    A small + on the portrait of any character with enough experience to
    train.

Item stats (ItemStats)
    Weapon damage and armor class are shown beside item names. The game
    normally only reveals these through Identify.

Space confirms (SpaceConfirms)
    Space and Enter answer yes on confirmation prompts.

Tab switches character (TabSwitchesCharacter)
    On screens where F1-F6 pick a character, Tab moves to the next one and
    Shift+Tab to the previous one.

Map notes (MapNotes)
    On the map screen (M), left-click a tile to attach a note, left-click a
    marker to edit it, right-click a marker to delete it. Hover over a marker
    to read it; V shows or hides the text of every note.

Map zoom and drag (MapZoom)
    On the map screen, the mouse wheel (or - and =) zooms between three
    levels, and dragging with the left button (or the arrow keys) moves the
    view.

Maximum starting stats (MaxStartStats)
    New characters roll 21 in every attribute.

Starter kit (StarterKit)
    Newly created characters get a basic weapon and armor for their class.
    Without this, new characters start with nothing.


Saved games
-----------

Saves are kept in the "saves" folder inside the install folder, and are left
alone if you uninstall.

Saves made with this build also load in standard ScummVM, but standard
ScummVM drops backpack items beyond the first nine per category and any map
notes.


Licence and source code
-----------------------

ScummVM is free software under the GNU General Public License, version 3 or
later; see COPYING.txt. This build is ScummVM's development code with the
mods above added to the Might and Magic engine. The modified source code is
available from whoever gave you this installer, as the licence requires.

Might and Magic and World of Xeen are trademarks of their respective owners.
This project is not affiliated with them or with the ScummVM team.
