#pragma once

#include <gtest/gtest.h>
#include "Core.h"
#include "Flags.h"
#include "PathFinding/ConfigurePath.h"
#include "PathFinding/NavalAStar.h"
#include "Point.h"

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
