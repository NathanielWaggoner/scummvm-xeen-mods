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
#define MAX_ZOOM 3

// Whether the text of every map note is shown, rather than only the one under the
// mouse. It's kept between visits to the map for as long as the game is running
static bool showAllNoteLabels = false;
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

	// Start with the standard view. With the map zoom option it can then be zoomed
	// and moved about, and with the map notes option tiles can be given notes
	_zoom = 1;
	_view = Common::Point(_pt.x - (MAP_DIFF - 1), _pt.y + (MAP_DIFF - 1));
	const bool canZoom = g_vm->_extOptions._mapZoom, canNote = g_vm->_extOptions._mapNotes;

	windows[5].open();
	bool drawFlag = true;
	bool finished = false;

#ifdef USE_TTS
	_vm->stopTextToSpeech();
#endif
	bool ttsVoiceText = true;

	events.takeWheelDelta();
	events.updateGameCounter();
	do {
		if (drawFlag)
			intf.draw3d(false, false);
		render(ttsVoiceText);

		events.ipause5(2);
		drawFlag = false;

		// The mouse wheel zooms, with wheel-up zooming in
		int wheel = events.takeWheelDelta();
		if (canZoom && wheel)
			setZoom(_zoom - wheel);

		// Otherwise any key or click closes the map, apart from the enhanced controls:
		// dragging moves the view, a left click on a tile adds or edits a note, a right
		// click removes one, plus/minus zoom, the arrow keys move the view, and V
		// switches the text of all the notes on and off
		PendingEvent event;
		while (!finished && events.getEvent(event)) {
			int mazeId;
			Common::Point tile, mapPos;

			if ((canZoom || canNote) && event.isMouse()
					&& tileAt(events._mousePos, mazeId, tile, mapPos)) {
				if (event._leftButton) {
					if (dragOrClick())
						drawFlag = true;
				} else {
					events.debounceMouse();
					int noteIdx = canNote ? party.findMapNote(mazeId, map._loadCcNum, tile) : -1;
					if (noteIdx != -1)
						party._mapNotes.remove_at(noteIdx);
				}
			} else if (canNote && event.isKeyboard() && event._keyState.keycode == Common::KEYCODE_v) {
				showAllNoteLabels = !showAllNoteLabels;
			} else if (canZoom && event.isKeyboard()) {
				const int step = viewTiles() / 8;
				switch (event._keyState.keycode) {
				case Common::KEYCODE_MINUS:
				case Common::KEYCODE_KP_MINUS:
					setZoom(_zoom + 1);
					break;
				case Common::KEYCODE_EQUALS:
				case Common::KEYCODE_PLUS:
				case Common::KEYCODE_KP_PLUS:
					setZoom(_zoom - 1);
					break;
				case Common::KEYCODE_LEFT:
					panView(-step, 0);
					break;
				case Common::KEYCODE_RIGHT:
					panView(step, 0);
					break;
				case Common::KEYCODE_UP:
					panView(0, step);
					break;
				case Common::KEYCODE_DOWN:
					panView(0, -step);
					break;
				default:
					finished = true;
					break;
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

int MapDialog::viewTiles() const {
	return MAP_SIZE * _zoom;
}

void MapDialog::clampView() {
	// Map positions run from -16 to 31 in each direction: the current maze, plus
	// the one beyond it on every side
	const int tiles = viewTiles();
	_view.x = CLIP<int>(_view.x, -16, 32 - tiles);
	_view.y = CLIP<int>(_view.y, tiles - 17, 31);
}

void MapDialog::setZoom(int zoom) {
	zoom = CLIP(zoom, 1, (int)MAX_ZOOM);
	if (zoom == _zoom)
		return;

	const int oldHalf = viewTiles() / 2;
	_zoom = zoom;
	const int newHalf = viewTiles() / 2;
	_view.x += oldHalf - newHalf;
	_view.y -= oldHalf - newHalf;
	clampView();
}

void MapDialog::panView(int dx, int dy) {
	_view.x += dx;
	_view.y += dy;
	clampView();
}

void MapDialog::drawView() {
	Map &map = *g_vm->_map;
	Screen &screen = *g_vm->_screen;
	Window &w = (*g_vm->_windows)[5];

	// The standard drawing code is reused as is. It works from the tile at the middle
	// of a 16 tile view, draws its own arrow when the blink flag is set, and advances
	// the water animation each time it's called, so those are all managed around it
	const Common::Point savedPt = _pt;
	const bool savedBlink = _frameEndFlag;
	const int animFrame = _animFrame;
	_frameEndFlag = false;

	// Grid area plus the strips to its left and top that edge walls are drawn in
	const Common::Rect gridArea(MAP_XSTART, MAP_YSTART,
		MAP_XSTART + MAP_SIZE * TILE_WIDTH, MAP_YSTART + MAP_SIZE * TILE_HEIGHT);
	const int stripX = 4, stripY = 8;
	const int cellW = gridArea.width(), cellH = gridArea.height();

	if (_zoom > 1 && screen.format.bytesPerPixel == 1) {
		// The window's background is a pattern, not one color, so what a block has
		// drawn is found by comparing against a copy of the cleared window
		const Common::Rect blockArea(gridArea.left - stripX, gridArea.top - stripY,
			gridArea.right, gridArea.bottom);
		const int bufferW = cellW * _zoom + stripX, bufferH = cellH * _zoom + stripY;

		w.writeString("\r");
		Graphics::ManagedSurface clean;
		clean.create(blockArea.width(), blockArea.height(), screen.format);
		clean.blitFrom(screen, blockArea, Common::Point(0, 0));

		if (_mapBuffer.w != bufferW) {
			_mapBuffer.free();
			_mapBuffer.create(bufferW, bufferH, screen.format);
		}
		Common::Array<byte> drawn;
		drawn.resize(bufferW * bufferH);
		Common::fill(drawn.begin(), drawn.end(), 0);

		// Draw each 16 tile block of the view in turn, collecting them side by side
		for (int blockY = 0; blockY < _zoom; ++blockY) {
			for (int blockX = 0; blockX < _zoom; ++blockX) {
				w.writeString("\r");
				_animFrame = animFrame;
				_pt = Common::Point(_view.x + blockX * MAP_SIZE + (MAP_DIFF - 1),
					_view.y - blockY * MAP_SIZE - (MAP_DIFF - 1));
				if (map._isOutdoors)
					drawOutdoors();
				else
					drawIndoors();

				// The edge strips are only kept for the blocks along the left and top of
				// the view. Elsewhere they'd leave stray marks over the neighbouring block
				const int firstX = blockX ? stripX : 0, firstY = blockY ? stripY : 0;

				for (int y = firstY; y < blockArea.height(); ++y) {
					const byte *src = (const byte *)screen.getBasePtr(blockArea.left, blockArea.top + y);
					const byte *cleanP = (const byte *)clean.getBasePtr(0, y);
					const int destY = blockY * cellH + y;
					byte *dest = (byte *)_mapBuffer.getBasePtr(blockX * cellW, destY);
					byte *drawnP = &drawn[destY * bufferW + blockX * cellW];

					for (int x = firstX; x < blockArea.width(); ++x) {
						if (src[x] != cleanP[x]) {
							dest[x] = src[x];
							drawnP[x] = 1;
						}
					}
				}
			}
		}

		// Shrink the collected blocks down into the grid area. Each screen pixel stands
		// for a square of the collected image; it takes the pixel at the square's
		// middle if that was drawn, or failing that any pixel in the square that was,
		// so that thin lines such as walls aren't lost
		w.writeString("\r");
		for (int y = 0; y < cellH; ++y) {
			byte *dest = (byte *)screen.getBasePtr(gridArea.left, gridArea.top + y);

			for (int x = 0; x < cellW; ++x) {
				const int srcX = stripX + x * _zoom, srcY = stripY + y * _zoom;
				int pick = (srcY + _zoom / 2) * bufferW + srcX + _zoom / 2;

				for (int subY = 0; subY < _zoom && !drawn[pick]; ++subY) {
					for (int subX = 0; subX < _zoom; ++subX) {
						const int idx = (srcY + subY) * bufferW + srcX + subX;
						if (drawn[idx]) {
							pick = idx;
							break;
						}
					}
				}

				if (drawn[pick])
					dest[x] = *(const byte *)_mapBuffer.getBasePtr(pick % bufferW, pick / bufferW);
			}
		}
		screen.addDirtyRect(gridArea);
	} else {
		_pt = Common::Point(_view.x + (MAP_DIFF - 1), _view.y - (MAP_DIFF - 1));
		if (map._isOutdoors)
			drawOutdoors();
		else
			drawIndoors();
	}

	_pt = savedPt;
	_frameEndFlag = savedBlink;
}

void MapDialog::render(bool &ttsVoiceText) {
	EventsManager &events = *g_vm->_events;
	Map &map = *g_vm->_map;
	Party &party = *g_vm->_party;
	Windows &windows = *g_vm->_windows;
	const bool enhanced = g_vm->_extOptions._mapZoom || g_vm->_extOptions._mapNotes;

	windows[5].writeString("\r");

	if (enhanced) {
		drawView();
		if (g_vm->_extOptions._mapNotes)
			drawNotes();
	} else if (map._isOutdoors) {
		drawOutdoors();
	} else {
		drawIndoors();
	}

	windows[5].frame();
	if (!map._isOutdoors)
		map._tileSprites.draw(0, 52, Common::Point(76, 30));

	if (enhanced) {
		// The party's arrow, wherever it now falls within the view
		const int tiles = viewTiles();
		const int col = party._mazePosition.x - _view.x, row = _view.y - party._mazePosition.y;
		if (_frameEndFlag && col >= 0 && col < tiles && row >= 0 && row < tiles) {
			// The arrow is drawn the size of a standard tile, centred on the party's tile
			const int xp = MAP_XSTART + (col * 2 + 1) * MAP_SIZE * TILE_WIDTH / (tiles * 2) - TILE_WIDTH / 2;
			const int yp = MAP_YSTART + (row * 2 + 1) * MAP_SIZE * TILE_HEIGHT / (tiles * 2) - TILE_HEIGHT / 2;
			_globalSprites.draw(0, party._mazeDirection + 1,
				Common::Point(xp + (map._isOutdoors ? 0 : 5), yp + (map._isOutdoors ? 0 : 4)));
		}
	} else if (map._isOutdoors && _frameEndFlag) {
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
}

bool MapDialog::dragOrClick() {
	EventsManager &events = *g_vm->_events;
	const Common::Point startMouse = events._mousePos;
	const Common::Point startView = _view;
	const int tiles = viewTiles();
	bool dragging = false;
	bool silent = false;

	// Follow the mouse for as long as the button is held. Once it's moved more than
	// a few pixels it's a drag, and the view is moved to keep up with it
	while (events.isMousePressed() && !g_vm->shouldExit()) {
		events.pollEventsAndWait();

		const Common::Point moved = events._mousePos - startMouse;
		if (!dragging && g_vm->_extOptions._mapZoom && (ABS(moved.x) > 3 || ABS(moved.y) > 3))
			dragging = true;

		if (dragging) {
			const Common::Point oldView = _view;
			_view = Common::Point(startView.x - moved.x * tiles / (MAP_SIZE * TILE_WIDTH),
				startView.y + moved.y * tiles / (MAP_SIZE * TILE_HEIGHT));
			clampView();

			if (_view != oldView)
				render(silent);
		}
	}
	events.clearEvents();

	if (dragging || !g_vm->_extOptions._mapNotes)
		return false;

	int mazeId;
	Common::Point tile, mapPos;
	if (!tileAt(startMouse, mazeId, tile, mapPos))
		return false;

	editNote(mazeId, tile, mapPos);
	return true;
}

bool MapDialog::tileAt(const Common::Point &screenPos, int &mazeId, Common::Point &tile,
		Common::Point &mapPos) {
	const Common::Rect area(MAP_XSTART, MAP_YSTART,
		MAP_XSTART + MAP_SIZE * TILE_WIDTH, MAP_YSTART + MAP_SIZE * TILE_HEIGHT);
	if (!area.contains(screenPos))
		return false;

	// North is at the top of the map
	const int tiles = viewTiles();
	tile.x = _view.x + (screenPos.x - MAP_XSTART) * tiles / area.width();
	tile.y = _view.y - (screenPos.y - MAP_YSTART) * tiles / area.height();
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
	const int tiles = viewTiles();
	const Common::Point mousePos = g_vm->_events->_mousePos;

	// Markers first, then the labels, so that a label is never hidden by a marker
	for (int pass = 0; pass < 2; ++pass) {
		for (int row = 0; row < tiles; ++row) {
			for (int col = 0; col < tiles; ++col) {
				Common::Point tile(_view.x + col, _view.y - row);
				int mazeId = map.mazeIdAt(tile);
				int noteIdx = mazeId ? party.findMapNote(mazeId, map._loadCcNum, tile) : -1;
				if (noteIdx == -1)
					continue;

				// The marker is a small outlined square over the middle of the tile
				const int xp = MAP_XSTART + (col * 2 + 1) * labelArea.width() / (tiles * 2) - 3;
				const int yp = MAP_YSTART + (row * 2 + 1) * labelArea.height() / (tiles * 2) - 3;
				if (pass == 0) {
					screen.fillRect(Common::Rect(xp, yp, xp + 6, yp + 6), _noteOutlineColor);
					screen.fillRect(Common::Rect(xp + 1, yp + 1, xp + 5, yp + 5), _noteColor);
					continue;
				}

				// A note's text is only shown while the mouse is over its marker, unless
				// all of them have been turned on
				if (!showAllNoteLabels && !Common::Rect(xp - 1, yp - 1, xp + 7, yp + 7).contains(mousePos))
					continue;

				// The label sits just above the marker, centred on it where there's room
				// and otherwise pushed back inside the grid. Long notes are cut short
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
