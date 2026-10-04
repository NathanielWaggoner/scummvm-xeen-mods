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
#include "mm/xeen/interface_minimap.h"
#include "mm/xeen/xeen.h"

namespace MM {
namespace Xeen {

#define MINIMAP_SIZE 7
#define MINIMAP_DIFF ((MINIMAP_SIZE - 1) / 2)
#define MINIMAP_XSTART 237
#define MINIMAP_YSTART 12
#define TILE_WIDTH 10
#define TILE_HEIGHT 8

void InterfaceMinimap::drawMinimap() {
	Map &map = *g_vm->_map;
	Party &party = *g_vm->_party;
	Resources &res = *g_vm->_resources;
	Windows &windows = *g_vm->_windows;

	if (windows[2]._enabled || windows[10]._enabled)
		return;
	if (!party._automapOn && !party._wizardEyeActive) {
		// Draw the Might & Magic logo
		if (g_vm->getGameID() == GType_Swords)
			res._logoSprites.draw(1, 0, Common::Point(MINIMAP_XSTART - (TILE_WIDTH / 2), 9));
		else
			res._globalSprites.draw(1, 5, Common::Point(MINIMAP_XSTART - (TILE_WIDTH / 2), 9));
		return;
	}

	bool eyeActive = party._wizardEyeActive;
	if (party._automapOn)
		party._wizardEyeActive = false;

	// Draw the minimap content
	if (map._isOutdoors)
		drawOutdoorsMinimap();
	else
		drawIndoorsMinimap();

	if (g_vm->_extOptions._mapNotes)
		drawMapNotes();

	// Draw outer rectangle around the automap
	res._globalSprites.draw(1, 6, Common::Point(223, 3));
	party._wizardEyeActive = eyeActive;
}

void InterfaceMinimap::drawMapNotes() {
	Map &map = *g_vm->_map;
	Party &party = *g_vm->_party;
	Screen &screen = *g_vm->_screen;
	if (party._mapNotes.empty())
		return;

	// Pick out a bright and a dark color for the markers from the current palette,
	// the same way the full map does
	byte palette[256 * 3];
	g_system->getPaletteManager()->grabPalette(palette, 0, 256);
	int bestLight = -1, bestDark = 0x7fffffff;
	byte noteColor = 0, outlineColor = 0;
	for (int idx = 0; idx < 256; ++idx) {
		const int r = palette[idx * 3], g = palette[idx * 3 + 1], b = palette[idx * 3 + 2];
		if (r + g - b > bestLight) {
			bestLight = r + g - b;
			noteColor = idx;
		}
		if (r + g + b < bestDark) {
			bestDark = r + g + b;
			outlineColor = idx;
		}
	}

	// The minimap is centred on the party, with north at the top
	for (int row = 0; row <= MINIMAP_DIFF * 2; ++row) {
		for (int col = 0; col <= MINIMAP_DIFF * 2; ++col) {
			// The party's own tile is left clear for its arrow
			if (row == MINIMAP_DIFF && col == MINIMAP_DIFF)
				continue;

			Common::Point tile(party._mazePosition.x - MINIMAP_DIFF + col,
				party._mazePosition.y + MINIMAP_DIFF - row);
			int mazeId = map.mazeIdAt(tile);
			if (!mazeId || party.findMapNote(mazeId, map._loadCcNum, tile) == -1)
				continue;

			const int xp = MINIMAP_XSTART + col * TILE_WIDTH + 2, yp = MINIMAP_YSTART + row * TILE_HEIGHT + 1;
			screen.fillRect(Common::Rect(xp, yp, xp + 6, yp + 6), outlineColor);
			screen.fillRect(Common::Rect(xp + 1, yp + 1, xp + 5, yp + 5), noteColor);
		}
	}
}

void InterfaceMinimap::drawOutdoorsMinimap() {
	Map &map = *g_vm->_map;
	Party &party = *g_vm->_party;
	Resources &res = *g_vm->_resources;
	int v, frame;
	const Common::Point &pt = party._mazePosition;

	res._globalSprites.draw(1, 15, Common::Point(MINIMAP_XSTART, MINIMAP_YSTART));

	for (int yp = MINIMAP_YSTART, mazeY = pt.y + MINIMAP_DIFF; mazeY >= (pt.y - MINIMAP_DIFF);
			yp += TILE_HEIGHT, --mazeY) {
		for (int xp = MINIMAP_XSTART, mazeX = pt.x - MINIMAP_DIFF; mazeX <= (pt.x + MINIMAP_DIFF);
				xp += TILE_WIDTH, ++mazeX) {
			v = map.mazeLookup(Common::Point(mazeX, mazeY), 0);
			assert(v != INVALID_CELL);
			frame = map.mazeDataCurrent()._surfaceTypes[v];

			if (map._currentSteppedOn || party._wizardEyeActive) {
				map._tileSprites.draw(1, frame, Common::Point(xp, yp));
			}
		}
	}

	for (int yp = MINIMAP_YSTART, mazeY = pt.y + MINIMAP_DIFF; mazeY >= (pt.y - MINIMAP_DIFF);
			yp += TILE_HEIGHT, --mazeY) {
		for (int xp = MINIMAP_XSTART, mazeX = pt.x - MINIMAP_DIFF; mazeX <= (pt.x + MINIMAP_DIFF);
				xp += TILE_WIDTH, ++mazeX) {
			v = map.mazeLookup(Common::Point(mazeX, mazeY), 4);
			assert(v != INVALID_CELL);
			frame = map.mazeData()._wallTypes[v];

			if (frame && (map._currentSteppedOn || party._wizardEyeActive)) {
				map._tileSprites.draw(1, frame + 16, Common::Point(xp, yp));
			}
		}
	}

	for (int yp = MINIMAP_YSTART, mazeY = pt.y + MINIMAP_DIFF; mazeY >= (pt.y - MINIMAP_DIFF);
			yp += TILE_HEIGHT, --mazeY) {
		for (int xp = MINIMAP_XSTART, mazeX = pt.x - MINIMAP_DIFF; mazeX <= (pt.x + MINIMAP_DIFF);
				xp += TILE_WIDTH, ++mazeX) {
			frame = map.mazeLookup(Common::Point(mazeX, mazeY), 8, 0xff);

			if (frame && (map._currentSteppedOn || party._wizardEyeActive)) {
				map._tileSprites.draw(1, frame + 32, Common::Point(xp, yp));
			}
		}
	}

	// Draw the direction arrow
	res._globalSprites.draw(1, party._mazeDirection + 1,
		Common::Point(267, 36));
}

void InterfaceMinimap::drawIndoorsMinimap() {
	Map &map = *g_vm->_map;
	Party &party = *g_vm->_party;
	Resources &res = *g_vm->_resources;
	int v, frame;
	const Common::Point &pt = party._mazePosition;
	int frame2 = _animFrame;
	_animFrame = (_animFrame + 2) % 8;

	// Draw default ground for all the valid explored areas
	for (int yp = MINIMAP_YSTART, mazeY = pt.y + MINIMAP_DIFF; mazeY >= (pt.y - MINIMAP_DIFF);
			yp += TILE_HEIGHT, --mazeY) {
		for (int xp = MINIMAP_XSTART, mazeX = pt.x - MINIMAP_DIFF; mazeX <= (pt.x + MINIMAP_DIFF);
				xp += TILE_WIDTH, ++mazeX) {
			v = map.mazeLookup(Common::Point(mazeX, mazeY), 0, 0xffff);

			if (v != INVALID_CELL && (map._currentSteppedOn || party._wizardEyeActive)) {
				map._tileSprites.draw(1, 0, Common::Point(xp, yp));
			}
		}
	}

	// Draw the specific surface type for each cell
	for (int yp = MINIMAP_YSTART + (TILE_HEIGHT / 2) + 1, mazeY = pt.y + MINIMAP_DIFF;
			mazeY >= (pt.y - MINIMAP_DIFF); yp += TILE_HEIGHT, --mazeY) {
		for (int xp = MINIMAP_XSTART + (TILE_WIDTH / 2), mazeX = pt.x - MINIMAP_DIFF;
				mazeX <= (pt.x + MINIMAP_DIFF); xp += TILE_WIDTH, ++mazeX) {
			v = map.mazeLookup(Common::Point(mazeX, mazeY), 0, 0xffff);
			int surfaceId = map.mazeData()._surfaceTypes[map._currentSurfaceId];

			if (v != INVALID_CELL && map._currentSurfaceId &&
				(map._currentSteppedOn || party._wizardEyeActive)) {
				map._tileSprites.draw(1, surfaceId + 36, Common::Point(xp, yp));
			}
		}
	}

	// Draw thin tile portion on top-left corner of map
	v = map.mazeLookup(Common::Point(pt.x - MINIMAP_DIFF - 1, pt.y + MINIMAP_DIFF + 1), 0, 0xffff);
	if (v != INVALID_CELL && map._currentSurfaceId &&
		(map._currentSteppedOn || party._wizardEyeActive)) {
		map._tileSprites.draw(1,
			map.mazeData()._surfaceTypes[map._currentSurfaceId] + 36,
			Common::Point(MINIMAP_XSTART - (TILE_WIDTH / 2),
				MINIMAP_YSTART - (TILE_HEIGHT / 2) + 1));
	}

	// Handle drawing surface sprites partially clipped at the left edge
	for (int yp = MINIMAP_YSTART + (TILE_HEIGHT / 2) + 1, mazeY = pt.y + MINIMAP_DIFF;
			mazeY >= (pt.y - MINIMAP_DIFF); yp += TILE_HEIGHT, --mazeY) {
		v = map.mazeLookup(Common::Point(pt.x - MINIMAP_DIFF - 1, mazeY), 0, 0xffff);

		if (v != INVALID_CELL && map._currentSurfaceId &&
			(map._currentSteppedOn || party._wizardEyeActive)) {
			map._tileSprites.draw(1,
				map.mazeData()._surfaceTypes[map._currentSurfaceId] + 36,
				Common::Point(MINIMAP_XSTART - (TILE_WIDTH / 2), yp));
		}
	}

	// Handle drawing surface sprites partially clipped at the top edge
	for (int xp = MINIMAP_XSTART + (TILE_WIDTH / 2), mazeX = pt.x - MINIMAP_DIFF;
			mazeX <= (pt.x + MINIMAP_DIFF); xp += TILE_WIDTH, ++mazeX) {
		v = map.mazeLookup(Common::Point(mazeX, pt.y + MINIMAP_DIFF + 1), 0, 0xffff);

		if (v != INVALID_CELL && map._currentSurfaceId &&
			(map._currentSteppedOn || party._wizardEyeActive)) {
			map._tileSprites.draw(1,
				map.mazeData()._surfaceTypes[map._currentSurfaceId] + 36,
				Common::Point(xp, MINIMAP_YSTART - (TILE_HEIGHT / 2) + 1));
		}
	}

	// Handle drawing partially clip top row and left column
	for (int xp = MINIMAP_XSTART, yp = MINIMAP_YSTART + (MINIMAP_SIZE - 1) * TILE_HEIGHT,
			mazeX = pt.x - MINIMAP_DIFF, mazeY = pt.y + MINIMAP_DIFF;
			mazeX <= (pt.x - MINIMAP_DIFF);
			xp += TILE_WIDTH, yp -= TILE_HEIGHT, ++mazeX, --mazeY) {
		// Left column
		v = map.mazeLookup(Common::Point(pt.x - MINIMAP_DIFF - 1, mazeY), 12, 0xffff);

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
			frame = 24;
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

		if (frame != -1 && (map._currentSteppedOn || party._wizardEyeActive))
			map._tileSprites.draw(1, frame, Common::Point(
				MINIMAP_XSTART - TILE_WIDTH - (TILE_WIDTH / 2), yp));

		// Top row
		v = map.mazeLookup(Common::Point(mazeX, pt.y + MINIMAP_DIFF + 1), 0);

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

		if (frame != -1 && (map._currentSteppedOn || party._wizardEyeActive))
			map._tileSprites.draw(1, frame, Common::Point(xp, MINIMAP_YSTART - TILE_HEIGHT));
	}

	// Draw the walls for the remaining cells of the minimap
	for (int yp = MINIMAP_YSTART, mazeY = pt.y + MINIMAP_DIFF; mazeY >= (pt.y - MINIMAP_DIFF);
			yp += TILE_HEIGHT, --mazeY) {
		for (int xp = MINIMAP_XSTART, mazeX = pt.x - MINIMAP_DIFF; mazeX <= (pt.x + MINIMAP_DIFF);
				xp += TILE_WIDTH, ++mazeX) {
			if (mazeX == pt.x && mazeY == pt.y) {
				// Center of the minimap. Draw the direction arrow
				res._globalSprites.draw(1, party._mazeDirection + 1,
					Common::Point(MINIMAP_XSTART + (TILE_WIDTH * 3) + (TILE_WIDTH / 2),
						MINIMAP_YSTART + (TILE_HEIGHT * 3) + (TILE_HEIGHT / 2)));
			}

			v = map.mazeLookup(Common::Point(mazeX, mazeY), 12, 0xffff);
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

			if (frame != -1 && (map._currentSteppedOn || party._wizardEyeActive)) {
				map._tileSprites.draw(1, frame, Common::Point(xp, yp));
			}

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

			if (frame != -1 && (map._currentSteppedOn || party._wizardEyeActive))
				map._tileSprites.draw(0, frame, Common::Point(xp, yp));
		}
	}

	// Draw overlay on cells that haven't been stepped on yet
	for (int yp = MINIMAP_YSTART, mazeY = pt.y + MINIMAP_DIFF; mazeY >= (pt.y - MINIMAP_DIFF);
			yp += TILE_HEIGHT, --mazeY) {
		for (int xp = MINIMAP_XSTART, mazeX = pt.x - MINIMAP_DIFF; mazeX <= (pt.x + MINIMAP_DIFF);
				xp += TILE_WIDTH, ++mazeX) {
			v = map.mazeLookup(
				Common::Point(mazeX, mazeY),
				0, 0xffff);

			if (v == INVALID_CELL || (!map._currentSteppedOn && !party._wizardEyeActive)) {
				map._tileSprites.draw(1, 1, Common::Point(xp, yp));
			}
		}
	}
}

} // End of namespace Xeen
} // End of namespace MM
