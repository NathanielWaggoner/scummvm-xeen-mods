/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef XEEN_DIALOGS_MAP_H
#define XEEN_DIALOGS_MAP_H

#include "mm/xeen/dialogs/dialogs.h"

namespace MM {
namespace Xeen {

class XeenEngine;

class MapDialog: public ButtonContainer {
private:
	int _animFrame;
	SpriteResource _globalSprites;
	Common::Point _pt, _arrowPt;
	bool _frameEndFlag;
	byte _noteColor, _noteOutlineColor;
private:
	MapDialog(XeenEngine *vm) : ButtonContainer(vm),
		_animFrame(0), _frameEndFlag(false) {}

	/**
	 * Draws the map contents when outdoors
	 */
	void drawOutdoors();

	/**
	 * Draws the map contents when indoors
	 */
	void drawIndoors();

	/**
	 * Works out which map tile is under a screen position: the maze it's in and its
	 * position there, plus its position as the map itself numbers things. Returns
	 * false if the position isn't over a tile of an actual maze
	 */
	bool tileAt(const Common::Point &screenPos, int &mazeId, Common::Point &tile,
		Common::Point &mapPos);

	/**
	 * Draws a marker on each visible tile that has a note
	 */
	void drawNotes();

	/**
	 * Prompts for the text of the note on a tile, adding the note if it's new
	 */
	void editNote(int mazeId, const Common::Point &tile, const Common::Point &mapPos);

	/**
	 * Handles the display of the dialog
	 */
	void execute();
public:
	static void show(XeenEngine *vm);
};

} // End of namespace Xeen
} // End of namespace MM

#endif
