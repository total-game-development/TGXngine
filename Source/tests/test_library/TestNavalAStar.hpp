#pragma once

#include <gtest/gtest.h>
#include "Core.h"
#include "Flags.h"
#include "PathFinding/ConfigurePath.h"
#include "PathFinding/NavalAStar.h"
#include "Point.h"

// Naval search tests.
//
// Grid convention matches the isle grid the ships path on: open water is
// CELL_COLLISION_MODE_OFF (0) and land is CELL_COLLISION_MODE_MEDIUM (100).
// The search returns the route end-to-start.

namespace
{
TGX::Vector<TGX::Vector<int>> OpenWater(int cols, int rows)
{
	return TGX::Vector<TGX::Vector<int>>(
		static_cast<size_t>(rows),
		TGX::Vector<int>(static_cast<size_t>(cols), TGX::Flags::CELL_COLLISION_MODE_OFF));
}

TGX::Vector<TGX::Point> RunNaval(
	const TGX::Vector<TGX::Vector<int>> &grid,
	TGX::Point start,
	TGX::Point end)
{
	TGX::Unique<TGX::ConfigurePath> configure = std::make_unique<TGX::NavalRoutesConfigurePath>();
	configure->codeMode = TGX::Flags::CELL_COLLISION_MODE_MEDIUM;

	TGX::NavalAStar naval;
	naval.Search(
		start,
		end,
		static_cast<int>(grid[0].size()),
		static_cast<int>(grid.size()),
		grid,
		configure);

	return naval.GetPath();
}

// Number of heading changes along the route. This is the property the turn
// penalty exists to minimise.
int CountTurns(const TGX::Vector<TGX::Point> &path)
{
	if (path.size() < 3)
	{
		return 0;
	}

	int turns = 0;

	for (size_t i = 2; i < path.size(); i++)
	{
		int previousX = path[i - 1].x - path[i - 2].x;
		int previousY = path[i - 1].y - path[i - 2].y;
		int currentX = path[i].x - path[i - 1].x;
		int currentY = path[i].y - path[i - 1].y;

		if (previousX != currentX || previousY != currentY)
		{
			turns++;
		}
	}

	return turns;
}
} // namespace

TEST(NavalAStar, StraightRunAcrossOpenWaterHasNoTurns)
{
	auto grid = OpenWater(40, 20);

	auto path = RunNaval(grid, {2, 10}, {30, 10});

	ASSERT_FALSE(path.empty());
	EXPECT_EQ(path.front(), TGX::Point(30, 10));
	EXPECT_EQ(CountTurns(path), 0);
}

// The cell the unit is standing on is not a step of the route. Handing it back
// makes the unit turn around and travel to its own cell centre before setting
// off, since it is almost never sitting exactly on that centre.
TEST(NavalAStar, RouteExcludesTheCellTheUnitStandsOn)
{
	auto grid = OpenWater(40, 20);

	auto path = RunNaval(grid, {2, 10}, {30, 10});

	ASSERT_FALSE(path.empty());
	EXPECT_EQ(path.back(), TGX::Point(3, 10));

	for (const auto &step : path)
	{
		EXPECT_FALSE(step.x == 2 && step.y == 10) << "route still contains the starting cell";
	}
}

// A unit ordered to the cell it already occupies has nowhere to go, and must
// not come back with a route consisting of that cell.
TEST(NavalAStar, RouteToTheCurrentCellIsEmpty)
{
	auto grid = OpenWater(40, 20);

	auto path = RunNaval(grid, {2, 10}, {2, 10});

	EXPECT_TRUE(path.empty());
}

TEST(NavalAStar, PerfectDiagonalHasNoTurns)
{
	auto grid = OpenWater(40, 40);

	auto path = RunNaval(grid, {2, 2}, {20, 20});

	ASSERT_FALSE(path.empty());
	EXPECT_EQ(CountTurns(path), 0);
}

// A route that is neither axis aligned nor a perfect diagonal has to mix two
// headings. Ordinary A* is free to interleave them step by step, since the
// distance is identical either way. The turn penalty should gather them into
// as few legs as it can rather than producing a staircase.
TEST(NavalAStar, ObliqueRunIsNotAStaircase)
{
	auto grid = OpenWater(60, 40);

	auto path = RunNaval(grid, {2, 2}, {40, 12});

	ASSERT_FALSE(path.empty());
	EXPECT_LE(CountTurns(path), 2);
}

TEST(NavalAStar, LandIsNotSailable)
{
	auto grid = OpenWater(20, 20);

	// Wall across the middle with a gap at the top.
	for (int x = 0; x < 15; x++)
	{
		grid[10][x] = TGX::Flags::CELL_COLLISION_MODE_MEDIUM;
	}

	auto path = RunNaval(grid, {2, 2}, {2, 18});

	ASSERT_FALSE(path.empty());

	for (const auto &step : path)
	{
		EXPECT_EQ(grid[step.y][step.x], TGX::Flags::CELL_COLLISION_MODE_OFF)
			<< "route crosses land at " << step.x << "," << step.y;
	}
}

TEST(NavalAStar, NoPathReturnsEmptyRatherThanCrashing)
{
	auto grid = OpenWater(20, 20);

	// Seal the destination behind land on every side.
	for (int x = 14; x < 18; x++)
	{
		grid[14][x] = TGX::Flags::CELL_COLLISION_MODE_MEDIUM;
		grid[18][x] = TGX::Flags::CELL_COLLISION_MODE_MEDIUM;
	}
	for (int y = 14; y < 19; y++)
	{
		grid[y][14] = TGX::Flags::CELL_COLLISION_MODE_MEDIUM;
		grid[y][17] = TGX::Flags::CELL_COLLISION_MODE_MEDIUM;
	}

	auto path = RunNaval(grid, {2, 2}, {16, 16});

	EXPECT_TRUE(path.empty());
}
