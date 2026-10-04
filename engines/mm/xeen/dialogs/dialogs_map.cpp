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

#include "graphics/paletteman.h"
#include "mm/xeen/dialogs/dialogs_map.h"
#include "mm/xeen/dialogs/dialogs_input.h"
#include "mm/xeen/resources.h"
#include "mm/xeen/xeen.h"

namespace MM {
namespace Xeen {

#define MAP_SIZE 16
#define MAP_DIFF (MAP_SIZE / 2)
#define MAP_XSTART 80
#define MAP_YSTART 38
#define TILE_WIDTH 10
#define TILE_HEIGHT 8
#define LABEL_PREFIX "\x03""l\x02\f""08"
#define LABEL_SUFFIX "\fd\x01"

void MapDialog::show(XeenEngine *vm) {
	MapDialog *dlg = new MapDialog(vm);
	dlg->execute();
	delete dlg;
}

void MapDialog::execute() {
	EventsManager &events = *_vm->_events;
	Interface &intf = *_vm->_interface;
	Map &map = *_vm->_map;
	Party &party = *_vm->_party;
	Windows &windows = *_vm->_windows;

	_pt = party._mazePosition;
	_globalSprites.load("global.icn");

	if (_pt.x < 8 && map.mazeData()._surroundingMazes._west == 0) {
		_arrowPt.x = _pt.x * 10 + 4;
		_pt.x = 7;
	} else if (_pt.x > 23) {
		_arrowPt.x = (byte)(_pt.x * 10 + 100);
		_pt.x = 23;
	} else if (_pt.x > 8 && map.mazeData()._surroundingMazes._east == 0) {
		_arrowPt.x = (byte)(_pt.x * 10 + 4);
		_pt.x = 7;
	} else {
		_arrowPt.x = 74;
	}

	if (_pt.y < 8 && map.mazeData()._surroundingMazes._south == 0) {
		_arrowPt.y = ((15 - _pt.y) << 3) + 13;
		_pt.y = 8;
	} else if (_pt.y > 24) {
		_arrowPt.y = ((15 - (_pt.y - 16)) << 3) + 13;
		_pt.y = 24;
	} else if (_pt.y >= 8 && map.mazeData()._surroundingMazes._north == 0) {
		_arrowPt.y = ((15 - _pt.y) << 3) + 13;
		_pt.y = 8;
	} else {
		_arrowPt.y = 69;
	}

	// Pick out marker colors for map notes from whatever palette is in use
	byte palette[256 * 3];
	g_system->getPaletteManager()->grabPalette(palette, 0, 256);
	int bestLight = -1, bestDark = 0x7fffffff;
	_noteColor = _noteOutlineColor = 0;
	for (int idx = 0; idx < 256; ++idx) {
		const int r = palette[idx * 3], g = palette[idx * 3 + 1], b = palette[idx * 3 + 2];
		const int yellowness = r + g - b;
		if (yellowness > bestLight) {
			bestLight = yellowness;
			_noteColor = idx;
		}
		if (r + g + b < bestDark) {
			bestDark = r + g + b;
			_noteOutlineColor = idx;
		}
	}

	windows[5].open();
	bool drawFlag = true;
	bool finished = false;

#ifdef USE_TTS
	_vm->stopTextToSpeech();
#endif
	bool ttsVoiceText = true;

	events.updateGameCounter();
	do {
		if (drawFlag)
			intf.draw3d(false, false);
		windows[5].writeString("\r");

		if (map._isOutdoors)
			drawOutdoors();
		else
			drawIndoors();

		if (g_vm->_extOptions._qualityOfLife)
			drawNotes();

		windows[5].frame();
		if (!map._isOutdoors) {
			map._tileSprites.draw(0, 52, Common::Point(76, 30));
		} else if (_frameEndFlag) {
			_globalSprites.draw(0, party._mazeDirection + 1,
				Common::Point(_arrowPt.x + 76, _arrowPt.y + 25));
		}

		if (events.timeElapsed() > 5) {
			// Set the flag to make the basic arrow blinking effect
			_frameEndFlag = !_frameEndFlag;
			events.updateGameCounter();
		}

		windows[5].writeString(Common::String::format(Res.MAP_TEXT,
			map._mazeName.c_str(), party._mazePosition.x,
			party._mazePosition.y, Res.DIRECTION_TEXT[party._mazeDirection]), ttsVoiceText);
		ttsVoiceText = false;
		windows[5].update();
		windows[3].update();

		events.ipause5(2);
		drawFlag = false;

		// Any key or click closes the map, other than clicks on map tiles when map
		// notes are available: left adds or edits a note, right removes one
		PendingEvent event;
		while (!finished && events.getEvent(event)) {
			int mazeId;
			Common::Point tile, mapPos;
			if (g_vm->_extOptions._qualityOfLife && event.isMouse()
					&& tileAt(events._mousePos, mazeId, tile, mapPos)) {
				events.debounceMouse();

				if (event._leftButton) {
					editNote(mazeId, tile, mapPos);
					drawFlag = true;
				} else {
					int noteIdx = party.findMapNote(mazeId, map._loadCcNum, tile);
					if (noteIdx != -1)
						party._mapNotes.remove_at(noteIdx);
				}
			} else {
				events.debounceMouse();
				finished = true;
			}
		}
	} while (!_vm->shouldExit() && !finished);

#ifdef USE_TTS
	_vm->stopTextToSpeech();
#endif
	events.clearEvents();
	windows[5].close();
}

bool MapDialog::tileAt(const Common::Point &screenPos, int &mazeId, Common::Point &tile,
		Common::Point &mapPos) {
	const Common::Rect area(MAP_XSTART, MAP_YSTART,
		MAP_XSTART + MAP_SIZE * TILE_WIDTH, MAP_YSTART + MAP_SIZE * TILE_HEIGHT);
	if (!area.contains(screenPos))
		return false;

	// The map shows MAP_SIZE tiles each way, with north at the top
	tile.x = _pt.x - (MAP_DIFF - 1) + (screenPos.x - MAP_XSTART) / TILE_WIDTH;
	tile.y = _pt.y + (MAP_DIFF - 1) - (screenPos.y - MAP_YSTART) / TILE_HEIGHT;
	mapPos = tile;
	mazeId = g_vm->_map->mazeIdAt(tile);
	return mazeId != 0;
}

void MapDialog::drawNotes() {
	Map &map = *g_vm->_map;
	Party &party = *g_vm->_party;
	Screen &screen = *g_vm->_screen;
	Window &w = (*g_vm->_windows)[5];

	// Labels are kept within the tile grid, clear of the window frame and title
	const Common::Rect labelArea(MAP_XSTART, MAP_YSTART,
		MAP_XSTART + MAP_SIZE * TILE_WIDTH, MAP_YSTART + MAP_SIZE * TILE_HEIGHT);
	enum { MAX_LABEL_CHARS = 24, LABEL_HEIGHT = 8 };

	// Markers first, then the labels, so that a label is never hidden by a marker
	for (int pass = 0; pass < 2; ++pass) {
		for (int row = 0; row < MAP_SIZE; ++row) {
			for (int col = 0; col < MAP_SIZE; ++col) {
				Common::Point tile(_pt.x - (MAP_DIFF - 1) + col, _pt.y + (MAP_DIFF - 1) - row);
				int mazeId = map.mazeIdAt(tile);
				int noteIdx = mazeId ? party.findMapNote(mazeId, map._loadCcNum, tile) : -1;
				if (noteIdx == -1)
					continue;

				// The marker is a small outlined square in the middle of the tile
				const int xp = MAP_XSTART + col * TILE_WIDTH + 2, yp = MAP_YSTART + row * TILE_HEIGHT + 1;
				if (pass == 0) {
					screen.fillRect(Common::Rect(xp, yp, xp + 6, yp + 6), _noteOutlineColor);
					screen.fillRect(Common::Rect(xp + 1, yp + 1, xp + 5, yp + 5), _noteColor);
					continue;
				}

				// The label sits just above the marker, centred on it where there's room
				// and otherwise pushed back inside the window. Long notes are cut short
				Common::String label = party._mapNotes[noteIdx]._text;
				if (label.size() > MAX_LABEL_CHARS)
					label = Common::String(label.c_str(), MAX_LABEL_CHARS - 2) + "..";

				const int width = w.getStringWidth(label, true);
				int labelX = CLIP<int>(xp + 3 - width / 2, labelArea.left + 1,
					MAX<int>(labelArea.right - width - 2, labelArea.left + 1));
				int labelY = yp - LABEL_HEIGHT - 1;
				if (labelY < labelArea.top)
					labelY = yp + 7;

				screen.fillRect(Common::Rect(labelX - 1, labelY - 1, labelX + width + 1, labelY + LABEL_HEIGHT),
					_noteOutlineColor);
				w._writePos = Common::Point(labelX, labelY);
				w.FontSurface::writeString(LABEL_PREFIX + label + LABEL_SUFFIX, labelArea, false);
			}
		}
	}
}

void MapDialog::editNote(int mazeId, const Common::Point &tile, const Common::Point &mapPos) {
	Map &map = *g_vm->_map;
	Party &party = *g_vm->_party;
	Window &w = (*g_vm->_windows)[6];

	int noteIdx = party.findMapNote(mazeId, map._loadCcNum, tile);
	Common::String text = (noteIdx == -1) ? Common::String() : party._mapNotes[noteIdx]._text;
	const Common::String original = text;

	w.open();
	w.writeString(Common::String::format("\r\x03""cNote for %d, %d\v024\t000", mapPos.x, mapPos.y));
	w.update();
	int length = Input::show(g_vm, &w, text, 30, 200);
	w.close();

	// Escape or an empty entry leaves things as they were
	if (!length || text == original)
		return;

	if (noteIdx == -1)
		party._mapNotes.push_back(MapNote(mazeId, map._loadCcNum, tile, text));
	else
		party._mapNotes[noteIdx]._text = text;
}

void MapDialog::drawOutdoors() {
	Map &map = *g_vm->_map;
	int v, frame;

	// Draw outdoors map
	for (int yp = MAP_YSTART, mazeY = _pt.y + MAP_DIFF - 1; mazeY >= (_pt.y - MAP_DIFF);
			--mazeY, yp += TILE_HEIGHT) {
		for (int xp = MAP_XSTART, mazeX = _pt.x - (MAP_DIFF - 1); mazeX <= (_pt.x + MAP_DIFF);
				xp += TILE_WIDTH, ++mazeX) {
			v = map.mazeLookup(Common::Point(mazeX, mazeY), 0);
			assert(v != INVALID_CELL);
			frame = map.mazeDataCurrent()._surfaceTypes[v];

			if (map._currentSteppedOn) {
				map._tileSprites.draw(0, frame, Common::Point(xp, yp));
			}
		}
	}

	for (int yp = MAP_YSTART, mazeY = _pt.y + MAP_DIFF - 1; mazeY >= (_pt.y - MAP_DIFF);
			--mazeY, yp += TILE_HEIGHT) {
		for (int xp = MAP_XSTART, mazeX = _pt.x - (MAP_DIFF - 1); mazeX <= (_pt.x + MAP_DIFF);
				xp += TILE_WIDTH, ++mazeX) {
			v = map.mazeLookup(Common::Point(mazeX, mazeY), 4);
			assert(v != INVALID_CELL);
			frame = map.mazeDataCurrent()._wallTypes[v];

			if (frame && map._currentSteppedOn)
				map._tileSprites.draw(0, frame + 16, Common::Point(xp, yp));
		}
	}

	for (int yp = MAP_YSTART, mazeY = _pt.y + MAP_DIFF - 1; mazeY >= (_pt.y - MAP_DIFF);
			--mazeY, yp += TILE_HEIGHT) {
		for (int xp = MAP_XSTART, mazeX = _pt.x - (MAP_DIFF - 1); mazeX <= (_pt.x + MAP_DIFF);
				xp += TILE_WIDTH, ++mazeX) {
			frame = map.mazeLookup(Common::Point(mazeX, mazeY), 8, 0xff);

			if (frame && map._currentSteppedOn)
				map._tileSprites.draw(0, frame + 32, Common::Point(xp, yp));
		}
	}
}

void MapDialog::drawIndoors() {
	Map &map = *g_vm->_map;
	Party &party = *g_vm->_party;
	int v, frame;
	int frame2 = _animFrame;
	_animFrame = (_animFrame + 2) % 8;

	// Draw indoors map
	frame2 = (frame2 + 2) % 8;

	// Draw default ground for all the valid explored areas
	for (int yp = MAP_YSTART, mazeY = _pt.y + MAP_DIFF - 1; mazeY >= (_pt.y - MAP_DIFF);
			yp += TILE_HEIGHT, --mazeY) {
		for (int xp = MAP_XSTART, mazeX = _pt.x - (MAP_DIFF - 1); mazeX <= (_pt.x + MAP_DIFF);
				xp += TILE_WIDTH, ++mazeX) {
			v = map.mazeLookup(Common::Point(mazeX, mazeY), 0, 0xffff);

			if (v != INVALID_CELL && map._currentSteppedOn)
				map._tileSprites.draw(0, 0, Common::Point(xp, yp));
		}
	}

	// Draw thinner ground tiles on the left edge of the map
	for (int yp = MAP_YSTART + 5, mazeY = _pt.y + MAP_DIFF - 1; mazeY >= (_pt.y - MAP_DIFF);
			yp += TILE_HEIGHT, --mazeY) {
		v = map.mazeLookup(Common::Point(_pt.x - 8, mazeY), 0, 0xffff);

		if (v != INVALID_CELL && map._currentSurfaceId != 0 && map._currentSteppedOn)
			map._tileSprites.draw(0, 36 + map.mazeData()._surfaceTypes[
				map._currentSurfaceId], Common::Point(75, yp));
	}

	// Draw thin tile portion on top-left corner of map
	v = map.mazeLookup(Common::Point(_pt.x - 8, _pt.y + 8), 0, 0xffff);
	if (v != INVALID_CELL && map._currentSurfaceId != 0 && map._currentSteppedOn)
		map._tileSprites.draw(0, 36 + map.mazeData()._surfaceTypes[
			map._currentSurfaceId], Common::Point(75, 35));

	// Draw any thin tiles at the very top of the map
	for (int xp = MAP_XSTART + 5, mazeX = _pt.x - (MAP_DIFF - 1); mazeX <= (_pt.x + MAP_DIFF);
			xp += TILE_WIDTH, ++mazeX) {
		v = map.mazeLookup(Common::Point(mazeX, _pt.y + 8), 0, 0xffff);

		if (v != INVALID_CELL && map._currentSurfaceId != 0 && map._currentSteppedOn)
			map._tileSprites.draw(0, 36 + map.mazeData()._surfaceTypes[
				map._currentSurfaceId], Common::Point(xp, 35));
	}

	// Draw the default ground tiles
	for (int yp = MAP_YSTART + 5, mazeY = _pt.y + MAP_DIFF - 1; mazeY >= (_pt.y - MAP_DIFF);
			yp += TILE_HEIGHT, --mazeY) {
		for (int xp = MAP_XSTART + 5, mazeX = _pt.x - (MAP_DIFF - 1); mazeX <= (_pt.x + MAP_DIFF);
				xp += TILE_WIDTH, ++mazeX) {
			v = map.mazeLookup(Common::Point(mazeX, mazeY), 0, 0xffff);

			if (v != INVALID_CELL && map._currentSurfaceId && map._currentSteppedOn)
				map._tileSprites.draw(0, 36 + map.mazeData()._surfaceTypes[
					map._currentSurfaceId], Common::Point(xp, yp));
		}
	}

	// Draw walls on left and top edges of map
	for (int xp = MAP_XSTART, yp = MAP_YSTART + (MAP_SIZE - 1) * TILE_HEIGHT,
			mazeX = _pt.x - (MAP_DIFF - 1), mazeY = _pt.y - MAP_DIFF;
			mazeX < (_pt.x + MAP_DIFF); xp += TILE_WIDTH, yp -= TILE_HEIGHT, ++mazeX, ++mazeY) {
		// Draw walls on left edge of map
		v = map.mazeLookup(Common::Point(_pt.x - 8, mazeY), 12);

		switch (v) {
		case SURFTYPE_DIRT:
			frame = 18;
			break;
		case SURFTYPE_SNOW:
			frame = 22;
			break;
		case SURFTYPE_SWAMP:
		case SURFTYPE_CLOUD:
			frame = 16;
			break;
		case SURFTYPE_LAVA:
		case SURFTYPE_DWATER:
			frame = 2;
			break;
		case SURFTYPE_DESERT:
			frame = 30;
			break;
		case SURFTYPE_ROAD:
			frame = 32;
			break;
		case SURFTYPE_TFLR:
			frame = 20;
			break;
		case SURFTYPE_SKY:
			frame = 28;
			break;
		case SURFTYPE_CROAD:
			frame = 14;
			break;
		case SURFTYPE_SEWER:
			frame = frame2 + 4;
			break;
		case SURFTYPE_SCORCH:
			frame = 24;
			break;
		case SURFTYPE_SPACE:
			frame = 26;
			break;
		default:
			frame = -1;
			break;
		}

		if (frame != -1 && map._currentSteppedOn)
			map._tileSprites.draw(0, frame, Common::Point(70, yp));

		// Draw walls on top edge of map
		v = map.mazeLookup(Common::Point(mazeX, _pt.y + 8), 0);

		switch (v) {
		case SURFTYPE_DIRT:
			frame = 19;
			break;
		case SURFTYPE_GRASS:
			frame = 35;
			break;
		case SURFTYPE_SNOW:
			frame = 23;
			break;
		case SURFTYPE_SWAMP:
		case SURFTYPE_CLOUD:
			frame = 17;
			break;
		case SURFTYPE_LAVA:
		case SURFTYPE_DWATER:
			frame = 3;
			break;
		case SURFTYPE_DESERT:
			frame = 31;
			break;
		case SURFTYPE_ROAD:
			frame = 33;
			break;
		case SURFTYPE_TFLR:
			frame = 21;
			break;
		case SURFTYPE_SKY:
			frame = 29;
			break;
		case SURFTYPE_CROAD:
			frame = 15;
			break;
		case SURFTYPE_SEWER:
			frame = frame2 + 5;
			break;
		case SURFTYPE_SCORCH:
			frame = 25;
			break;
		case SURFTYPE_SPACE:
			frame = 27;
			break;
		default:
			frame = -1;
			break;
		}

		if (frame != -1 && map._currentSteppedOn)
			map._tileSprites.draw(0, frame, Common::Point(xp, 30));
	}

	// Draw the walls for the remaining cells of the minimap
	for (int yp = MAP_YSTART, mazeY = _pt.y + MAP_DIFF - 1, yCtr = 0; yCtr < MAP_SIZE;
			yp += TILE_HEIGHT, --mazeY, ++yCtr) {
		for (int xp = MAP_XSTART, mazeX = _pt.x - (MAP_DIFF - 1), xCtr = 0; xCtr < MAP_SIZE;
				xp += TILE_WIDTH, ++mazeX, ++xCtr) {
			// Draw the arrow if at the correct position
			if ((_arrowPt.x / 10) == xCtr && (14 - (_arrowPt.y / 10)) == yCtr && _frameEndFlag) {
				_globalSprites.draw(0, party._mazeDirection + 1,
					Common::Point(_arrowPt.x + 81, _arrowPt.y + 29));
			}

			v = map.mazeLookup(Common::Point(mazeX, mazeY), 12);
			switch (v) {
			case 1:
				frame = 18;
				break;
			case 2:
				frame = 34;
				break;
			case 3:
				frame = 22;
				break;
			case 4:
			case 13:
				frame = 16;
				break;
			case 5:
			case 8:
				frame = 2;
				break;
			case 6:
				frame = 30;
				break;
			case 7:
				frame = 32;
				break;
			case 9:
				frame = 20;
				break;
			case 10:
				frame = 28;
				break;
			case 11:
				frame = 14;
				break;
			case 12:
				frame = frame2 + 4;
				break;
			case 14:
				frame = 24;
				break;
			case 15:
				frame = 26;
				break;
			default:
				frame = -1;
				break;
			}

			if (frame != -1 && map._currentSteppedOn)
				map._tileSprites.draw(0, frame, Common::Point(xp, yp));

			v = map.mazeLookup(Common::Point(mazeX, mazeY), 0);
			switch (v) {
			case 1:
				frame = 19;
				break;
			case 2:
				frame = 35;
				break;
			case 3:
				frame = 23;
				break;
			case 4:
			case 13:
				frame = 17;
				break;
			case 5:
			case 8:
				frame = 3;
				break;
			case 6:
				frame = 31;
				break;
			case 7:
				frame = 33;
				break;
			case 9:
				frame = 21;
				break;
			case 10:
				frame = 29;
				break;
			case 11:
				frame = 15;
				break;
			case 12:
				frame = frame2 + 5;
				break;
			case 14:
				frame = 25;
				break;
			case 15:
				frame = 27;
				break;
			default:
				frame = -1;
				break;
			}

			if (frame != -1 && map._currentSteppedOn)
				map._tileSprites.draw(0, frame, Common::Point(xp, yp));
		}
	}

	// Draw overlay on cells that haven't been stepped on yet
	for (int yp = MAP_YSTART, mazeY = _pt.y + MAP_DIFF - 1; mazeY >= (_pt.y - MAP_DIFF);
			yp += TILE_HEIGHT, --mazeY) {
		for (int xp = MAP_XSTART, mazeX = _pt.x - (MAP_DIFF - 1); mazeX <= (_pt.x + MAP_DIFF);
				xp += TILE_WIDTH, ++mazeX) {
			v = map.mazeLookup(Common::Point(mazeX, mazeY), 0, 0xffff);

			if (v == INVALID_CELL || !map._currentSteppedOn)
				map._tileSprites.draw(0, 1, Common::Point(xp, yp));
		}
	}
}

} // End of namespace Xeen
} // End of namespace MM
