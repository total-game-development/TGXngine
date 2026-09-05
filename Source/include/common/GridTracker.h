#pragma once

#include "Core.h"

namespace TGX
{
// A cell a unit has booked to stop on, so the next unit's search routes around
// it rather than halting in the same place. Kept apart from uids_grid: a unit
// holds a booking and a body at once, and the body is what has to go back on
// the grid when it arrives.
struct TacticalReservation
{
	int x1 = 0;
	int y1 = 0;
	int x2 = 0;
	int y2 = 0;
	int cellMode = 0;
};

struct GridTracker
{
	Map<int, std::tuple<int, int, int, int>> uids_grid;
	Map<String, int> cells_grid;
	Map<int, TacticalReservation> tactical_uids_grid;

	GridTracker() = default;

	void Clear()
	{
		uids_grid.clear();
		cells_grid.clear();
		tactical_uids_grid.clear();
	}
};
} // namespace TGX
