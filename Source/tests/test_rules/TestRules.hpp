#pragma once

#include <gtest/gtest.h>

#include "Flags.h"
#include "Physics.h"
#include "Rules.h"
#include "WorldState.h"

namespace
{
using namespace TGX;

class RulesFixture : public ::testing::Test
{
protected:
	static constexpr int SIDE = 8;

	void SetUp() override
	{
		WorldState &world = WorldState::GetInstance();

		world.items.clear();
		world.SetMapGridWidth(SIDE);
		world.SetMapGridHeight(SIDE);
		world.currentTerrainMapPassableGrid.assign(SIDE, Vector<int>(SIDE, Flags::CELL_COLLISION_MODE_OFF));

		Physics::GetInstance().GetGridTracker().Clear();
	}

	void Hold(int x, int y, int stack, int mark)
	{
		Physics::GetInstance().GetGridTracker().cells_grid[std::to_string(x) + " " + std::to_string(y)] = stack;
		WorldState::GetInstance().currentTerrainMapPassableGrid[y][x] = mark;
	}

	static bool Caught(const Vector<Rules::Violation> &found, const String &rule, const String &text)
	{
		for (const Rules::Violation &violation : found)
		{
			if (violation.rule == rule && violation.detail.find(text) != String::npos)
			{
				return true;
			}
		}

		return false;
	}
};

TEST_F(RulesFixture, AnEmptyGridBreaksNothing)
{
	EXPECT_TRUE(Rules::Check().empty());
}

TEST_F(RulesFixture, InfantryStoppedOnAVehicleIsCaught)
{
	Hold(3, 4, Flags::CELL_COLLISION_MODE_MEDIUM + Flags::CELL_COLLISION_MODE_SOFT, Flags::CELL_COLLISION_MODE_MEDIUM);

	EXPECT_TRUE(Caught(Rules::Check(), "overlap", "infantry stopped where a vehicle"));
}

TEST_F(RulesFixture, TwoVehiclesInOneCellAreCaught)
{
	Hold(2, 2, Flags::CELL_COLLISION_MODE_MEDIUM * 2, Flags::CELL_COLLISION_MODE_MEDIUM);

	EXPECT_TRUE(Caught(Rules::Check(), "overlap", "2 vehicles or ships stopped on it"));
}

TEST_F(RulesFixture, AUnitStoppedInsideABuildingIsCaught)
{
	Hold(5, 1, Flags::CELL_COLLISION_MODE_HARD + Flags::CELL_COLLISION_MODE_SOFT, Flags::CELL_COLLISION_MODE_HARD);

	EXPECT_TRUE(Caught(Rules::Check(), "overlap", "stopped inside a building"));
}

TEST_F(RulesFixture, OneVehicleAloneBreaksNothing)
{
	Hold(6, 6, Flags::CELL_COLLISION_MODE_MEDIUM, Flags::CELL_COLLISION_MODE_MEDIUM);

	const Vector<Rules::Violation> found = Rules::Check();

	EXPECT_FALSE(Caught(found, "overlap", "cell 6 6"));
	EXPECT_FALSE(Caught(found, "grid", "cell 6 6"));
}

TEST_F(RulesFixture, AStackNoBodyHoldsIsCaught)
{
	Hold(1, 1, Flags::CELL_COLLISION_MODE_SOFT, Flags::CELL_COLLISION_MODE_SOFT);

	EXPECT_TRUE(Caught(Rules::Check(), "stack", "with no body on it"));
}

TEST_F(RulesFixture, AStackBelowNothingIsCaught)
{
	Hold(0, 7, -1, Flags::CELL_COLLISION_MODE_OFF);

	EXPECT_TRUE(Caught(Rules::Check(), "stack", "which no body can have put there"));
}

TEST_F(RulesFixture, AMarkTheStackDoesNotSupportIsCaught)
{
	Hold(4, 4, Flags::CELL_COLLISION_MODE_MEDIUM, Flags::CELL_COLLISION_MODE_OFF);

	EXPECT_TRUE(Caught(Rules::Check(), "grid", "rather than " + std::to_string(Flags::CELL_COLLISION_MODE_MEDIUM)));
}

TEST_F(RulesFixture, ABodyNothingAliveCarriesIsCaught)
{
	Physics::GetInstance().GetGridTracker().uids_grid[77] = {1, 1, 2, 2};

	EXPECT_TRUE(Caught(Rules::Check(), "body", "uid 77"));
}

TEST_F(RulesFixture, ABookingNothingAliveCarriesIsCaught)
{
	Physics::GetInstance().GetGridTracker().tactical_uids_grid[91] = {3, 3, 4, 4, Flags::CELL_COLLISION_MODE_MEDIUM};

	EXPECT_TRUE(Caught(Rules::Check(), "booking", "uid 91"));
}
} // namespace
