#pragma once

#include <Common.hpp>
#include "ConfigurePath.h"
#include "Core.h"
#include "IPathfinding.h"
#include "Point.h"

namespace TGX
{
/*
 * A* variant for naval units.
 *
 * Shares the grid conventions, collision handling and neighbour model of the
 * plain A*, but does not reuse it. Hulls cannot pivot on the spot the way
 * infantry and vehicles can, so a route full of single-step zig-zags — which
 * costs an ordinary A* exactly the same as a straight run, since only distance
 * is counted — reads as wrong and is unsailable.
 *
 * This search charges a turn penalty: continuing on the heading that entered a
 * cell is free, changing heading adds to g. The result favours long straight
 * legs and turns only where it must.
 */
class NavalAStar : public IPathfinding
{
public:
	void Search(
		const Point &start,
		const Point &end,
		int cols, int rows,
		const Vector<Vector<int>> &grid,
		const Unique<ConfigurePath> &configure) override;

	Vector<Point> GetPath() const override;

private:
	Vector<Point> path;
};
} // namespace TGX
