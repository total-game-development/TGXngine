#pragma once

#include <Common.hpp>
#include "ConfigurePath.h"
#include "Core.h"
#include "IPathfinding.h"
#include "Point.h"

namespace TGX
{
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
