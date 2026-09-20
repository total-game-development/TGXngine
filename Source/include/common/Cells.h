#pragma once

#include <algorithm>
#include <cmath>
#include "CollisionStructures.h"
#include "GridTracker.h"
#include "Core.h"
#include "Flags.h"
#include "Globals.h"

namespace TGX
{
class Cells
{
public:
	inline static Map<int, CollisionCoords> tactical_uids_grid;

	inline static Map<String, int> tactical_grid;

	static void Init()
	{
		tactical_uids_grid.clear();
		tactical_grid.clear();
	}

	// Ground a unit may come to rest on. Infantry may share a cell with
	// infantry; anything that takes a whole cell needs one nobody holds, and
	// nothing at all may stop where such a thing already stands. The unit's own
	// booking is discounted, since the cell it booked is the one it is stopping
	// on.
	static bool Standable(
		int cellX, int cellY, int cellMode, int ownBooking,
		const Vector<Vector<int>> &passableGrid,
		const Map<String, int> &cells_grid)
	{
		if (cellY < 0 || cellY >= static_cast<int>(passableGrid.size()))
		{
			return false;
		}

		if (cellX < 0 || cellX >= static_cast<int>(passableGrid[cellY].size()))
		{
			return false;
		}

		if (passableGrid[cellY][cellX] >= Flags::CELL_COLLISION_MODE_HARD)
		{
			return false;
		}

		const auto held = cells_grid.find(std::to_string(cellX) + " " + std::to_string(cellY));
		const int stack = (held == cells_grid.end() ? 0 : held->second) - ownBooking;

		if (cellMode >= Flags::CELL_COLLISION_MODE_MEDIUM)
		{
			return stack <= Flags::CELL_COLLISION_MODE_OFF;
		}

		return stack < Flags::CELL_COLLISION_MODE_MEDIUM;
	}

	// Where a unit stopping at (item_x, item_y) may put its body down: the spot
	// it asked for when that is free, otherwise the nearest that is, searched
	// outwards in a fixed order so every client settles it the same way. A unit
	// hemmed in altogether keeps the spot it asked for, and the rule audit says
	// so.
	static Pair<float, float> Settle(
		int uid, float item_x, float item_y, float radius, int cellMode,
		const Vector<Vector<int>> &passableGrid,
		const Map<int, TacticalReservation> &bookings,
		const Map<String, int> &cells_grid,
		int reach = 4)
	{
		const auto booked = bookings.find(uid);

		const auto clear = [&](float atX, float atY) {
			const int x1 = static_cast<int>(std::floor(atX - radius));
			const int x2 = static_cast<int>(std::floor(atX + radius));
			const int y1 = static_cast<int>(std::floor(atY - radius));
			const int y2 = static_cast<int>(std::floor(atY + radius));

			for (int x = x1; x <= x2; ++x)
			{
				for (int y = y1; y <= y2; ++y)
				{
					int ownBooking = 0;

					if (booked != bookings.end() &&
						x >= booked->second.x1 && x <= booked->second.x2 &&
						y >= booked->second.y1 && y <= booked->second.y2)
					{
						ownBooking = booked->second.cellMode;
					}

					if (!Standable(x, y, cellMode, ownBooking, passableGrid, cells_grid))
					{
						return false;
					}
				}
			}

			return true;
		};

		if (clear(item_x, item_y))
		{
			return {item_x, item_y};
		}

		for (int ring = 1; ring <= reach; ring++)
		{
			for (int offsetY = -ring; offsetY <= ring; offsetY++)
			{
				for (int offsetX = -ring; offsetX <= ring; offsetX++)
				{
					if (std::abs(offsetX) != ring && std::abs(offsetY) != ring)
					{
						continue;
					}

					const float atX = item_x + static_cast<float>(offsetX);
					const float atY = item_y + static_cast<float>(offsetY);

					if (clear(atX, atY))
					{
						return {atX, atY};
					}
				}
			}
		}

		return {item_x, item_y};
	}

	static void Add(
		int uid, float item_x, float item_y, float radius,
		int mapGridWidth, int mapGridHeight,
		Vector<Vector<int>> &passableGrid,
		Map<int, std::tuple<int, int, int, int>> &uids_grid,
		Map<String, int> &cells_grid,
		int cellMode = 0)
	{
		if (uids_grid.contains(uid))
		{
			return;
		}

		int x1 = std::max(static_cast<int>(std::floor(item_x - radius)), 0);
		int x2 = std::min(static_cast<int>(std::floor(item_x + radius)), mapGridWidth - 1);
		int y1 = std::max(static_cast<int>(std::floor(item_y - radius)), 0);
		int y2 = std::min(static_cast<int>(std::floor(item_y + radius)), mapGridHeight - 1);

		for (int x = x1; x <= x2; ++x)
		{
			for (int y = y1; y <= y2; ++y)
			{
				if (passableGrid[y][x] < Flags::CELL_COLLISION_MODE_HARD)
				{
					std::string cellKey = std::to_string(x) + " " + std::to_string(y);

					if (cells_grid.contains(cellKey))
					{
						int cellStack = cells_grid[cellKey];
						cellStack += cellMode;
						cells_grid[cellKey] = cellStack;
					}
					else
					{
						cells_grid[cellKey] = cellMode;
					}

					int cellStack = cells_grid[cellKey];
					if (cellStack >= Flags::CELL_COLLISION_MODE_MEDIUM && cellStack < Flags::CELL_COLLISION_MODE_HARD)
					{
						passableGrid[y][x] = Flags::CELL_COLLISION_MODE_MEDIUM;
					}
					else if (cellStack > Flags::CELL_COLLISION_MODE_OFF)
					{
						passableGrid[y][x] = Flags::CELL_COLLISION_MODE_SOFT;
					}
				}
			}
		}

		uids_grid[uid] = {x1, y1, x2, y2};
	}

	static void Remove(
		int uid, Vector<Vector<int>> &passableGrid,
		Map<int, std::tuple<int, int, int, int>> &uids_grid,
		Map<String, int> &cells_grid, int cellMode = 0)
	{
		auto it = uids_grid.find(uid);
		if (it == uids_grid.end())
		{
			return;
		}

		int x1 = std::get<0>(it->second);
		int y1 = std::get<1>(it->second);
		int x2 = std::get<2>(it->second);
		int y2 = std::get<3>(it->second);

		for (int x = x1; x <= x2; ++x)
		{
			for (int y = y1; y <= y2; ++y)
			{
				if (passableGrid[y][x] >= Flags::CELL_COLLISION_MODE_SOFT)
				{
					String cellKey = std::to_string(x) + " " + std::to_string(y);

					auto cellIt = cells_grid.find(cellKey);

					if (cellIt != cells_grid.end())
					{
						int cellStack = cellIt->second;
						cellStack -= cellMode;

						cells_grid[cellKey] = cellStack;

						if (cellStack == Flags::CELL_COLLISION_MODE_OFF)
						{
							passableGrid[y][x] = Flags::CELL_COLLISION_MODE_OFF;
						}
						else if (cellStack < Flags::CELL_COLLISION_MODE_MEDIUM)
						{
							passableGrid[y][x] = Flags::CELL_COLLISION_MODE_SOFT;
						}
					}
				}
			}
		}

		uids_grid.erase(uid);
	}

	Vector<int> TakeSnapshot(float item_x, float item_y, const CollisionGrid &collisionGrid, Vector<Vector<int>> &grid)
	{
		int x1, x2, y1, y2;

		x1 = x2 = y1 = y2 = 0;

		if (collisionGrid.radius > 0)
		{
			x1 = std::max(static_cast<int>(std::floor(item_x - collisionGrid.radius)), 0);
			x2 = std::min(static_cast<int>(std::floor(item_x + collisionGrid.radius)), Globals::mapGridWidth - 1);

			y1 = std::max(static_cast<int>(std::floor(item_y - collisionGrid.radius)), 0);
			y2 = std::min(static_cast<int>(std::floor(item_y + collisionGrid.radius)), Globals::mapGridHeight - 1);
		}
		else
		{
			x1 = std::max(static_cast<int>(std::floor(item_x - static_cast<float>(collisionGrid.gridX))), 0);
			x2 = std::min(static_cast<int>(std::floor(item_x + static_cast<float>(collisionGrid.gridX))), Globals::mapGridWidth - 1);

			y1 = std::max(static_cast<int>(std::floor(item_y - static_cast<float>(collisionGrid.gridY))), 0);
			y2 = std::min(static_cast<int>(std::floor(item_y + static_cast<float>(collisionGrid.gridY))), Globals::mapGridHeight - 1);
		}

		Vector<int> current_tiles;

		for (int x = x1; x <= x2; ++x)
		{
			for (int y = y1; y <= y2; ++y)
			{
				current_tiles.push_back(grid[y][x]);

				grid[y][x] = Flags::CELL_COLLISION_MODE_OFF;
			}
		}

		return current_tiles;
	}

	void RestoreSnapshot(float item_x, float item_y, const CollisionGrid &collisionGrid,
						 Vector<Vector<int>> &grid, Vector<int> &current_tiles)
	{
		int x1 = 0, x2 = 0, y1 = 0, y2 = 0;

		if (collisionGrid.radius > 0.0)
		{
			x1 = std::max(static_cast<int>(std::floor(item_x - collisionGrid.radius)), 0);
			x2 = std::min(static_cast<int>(std::floor(item_x + collisionGrid.radius)), Globals::mapGridWidth - 1);

			y1 = std::max(static_cast<int>(std::floor(item_y - collisionGrid.radius)), 0);
			y2 = std::min(static_cast<int>(std::floor(item_y + collisionGrid.radius)), Globals::mapGridHeight - 1);
		}
		else
		{
			x1 = std::max(static_cast<int>(std::floor(item_x - static_cast<float>(collisionGrid.gridX))), 0);
			x2 = std::min(static_cast<int>(std::floor(item_x + static_cast<float>(collisionGrid.gridX))), Globals::mapGridWidth - 1);

			y1 = std::max(static_cast<int>(std::floor(item_y - static_cast<float>(collisionGrid.gridY))), 0);
			y2 = std::min(static_cast<int>(std::floor(item_y + static_cast<float>(collisionGrid.gridY))), Globals::mapGridHeight - 1);
		}

		for (int x = x1; x <= x2; x++)
		{
			for (int y = y1; y <= y2; y++)
			{
				if (!current_tiles.empty())
				{
					grid[y][x] = current_tiles.front();
					current_tiles.erase(current_tiles.begin());
				}
			}
		}
	}

	CoordsSnapshot TakeCoordsSnapshot(
		const Pair<int, int> &start, const Pair<int, int> &end, Vector<Vector<int>> &grid)
	{
		int startX = start.first;
		int startY = start.second;
		int startValue = grid[startY][startX];
		grid[startY][startX] = Flags::CELL_COLLISION_MODE_OFF;

		int endX = end.first;
		int endY = end.second;
		int endValue = grid[endY][endX];

		if (endValue != Flags::CELL_COLLISION_MODE_FULL)
		{
			grid[endY][endX] = Flags::CELL_COLLISION_MODE_OFF;
		}

		return CoordsSnapshot(startX, startY, startValue, endX, endY, endValue);
	}

	void RestoreCoordsSnapshot(
		const CoordsSnapshot &coordsSnapshot, Vector<Vector<int>> &grid)
	{
		int startX, startY, startValue, endX, endY, endValue;

		startX = coordsSnapshot.startX;
		startY = coordsSnapshot.startY;
		startValue = coordsSnapshot.startValue;

		endX = coordsSnapshot.endX;
		endY = coordsSnapshot.endY;
		endValue = coordsSnapshot.endValue;

		if (startValue != 0)
		{
			grid[startY][startX] = startValue;
		}

		if (endValue != 0 && grid[endY][endX] != Flags::CELL_COLLISION_MODE_FULL)
		{
			grid[endY][endX] = endValue;
		}
	}

	Cells() = delete;
	~Cells() = delete;
};
} // namespace TGX
