#pragma once

#include <gtest/gtest.h>

#include "Flags.h"
#include "Physics.h"
#include "Rules.h"
#include "WorldState.h"

namespace
{
using namespace TGX;

class StandingUnit : public ItemInstance
{
public:
	int mode = Flags::CELL_COLLISION_MODE_SOFT;

	int GetFrames() const override
	{
		return 1;
	}

	float GetRadius() const override
	{
		return 10.0f;
	}

	int GetCellCollisionMode() const override
	{
		return mode;
	}

	void AddToGrid(Vector<Vector<int>> &grid, GridTracker &gridTracker) const override
	{
	}

	void RemoveFromGrid(Vector<Vector<int>> &grid, GridTracker &gridTracker) const override
	{
	}
};

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

	// A unit in the world with no body on the grid, which is what a unit on
	// the move is: its body goes back down when it stops.
	void Alive(int uid, int mode, const String &name)
	{
		auto unit = std::make_unique<StandingUnit>();

		unit->mode = mode;
		unit->SetUid(uid);
		unit->SetName(name);
		unit->SetLife(100.0f);

		WorldState::GetInstance().items.push_back(std::move(unit));
	}

	// A body on one cell: alive, on the grid, and counted in the stack, which
	// is what the engine leaves behind when a unit comes to a stop.
	void Stand(int uid, int x, int y, int mode, const String &name)
	{
		Alive(uid, mode, name);

		Physics::GetInstance().GetGridTracker().uids_grid[uid] = {x, y, x, y};

		Hold(x, y, Stack(x, y) + mode);
	}

	void Book(int uid, int x, int y, int mode)
	{
		GridTracker &tracker = Physics::GetInstance().GetGridTracker();

		tracker.tactical_uids_grid[uid] = {x, y, x, y, mode};

		Hold(x, y, Stack(x, y) + mode);
	}

	int Stack(int x, int y) const
	{
		const GridTracker &tracker = Physics::GetInstance().GetGridTracker();
		const auto held = tracker.cells_grid.find(std::to_string(x) + " " + std::to_string(y));

		return held == tracker.cells_grid.end() ? 0 : held->second;
	}

	// The stack, and the mark the engine keeps in step with it.
	void Hold(int x, int y, int stack)
	{
		Physics::GetInstance().GetGridTracker().cells_grid[std::to_string(x) + " " + std::to_string(y)] = stack;

		Mark(x, y, stack >= Flags::CELL_COLLISION_MODE_MEDIUM	? Flags::CELL_COLLISION_MODE_MEDIUM
					: stack > Flags::CELL_COLLISION_MODE_OFF	? Flags::CELL_COLLISION_MODE_SOFT
															 : Flags::CELL_COLLISION_MODE_OFF);
	}

	void Mark(int x, int y, int mark)
	{
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

TEST_F(RulesFixture, AUnitStandingAloneBreaksNothing)
{
	Stand(1, 3, 3, Flags::CELL_COLLISION_MODE_MEDIUM, "tank");

	EXPECT_TRUE(Rules::Check().empty());
}

TEST_F(RulesFixture, InfantryStoppedOnAVehicleIsCaught)
{
	Stand(1, 3, 4, Flags::CELL_COLLISION_MODE_MEDIUM, "tank");
	Stand(2, 3, 4, Flags::CELL_COLLISION_MODE_SOFT, "rifleman");

	EXPECT_TRUE(Caught(Rules::Check(), "overlap", "tank#1 and rifleman#2 stopped on it"));
}

TEST_F(RulesFixture, TwoVehiclesInOneCellAreCaught)
{
	Stand(1, 2, 2, Flags::CELL_COLLISION_MODE_MEDIUM, "tank");
	Stand(2, 2, 2, Flags::CELL_COLLISION_MODE_MEDIUM, "truck");

	EXPECT_TRUE(Caught(Rules::Check(), "overlap", "stopped on it"));
}

TEST_F(RulesFixture, InfantrySharingACellIsAllowed)
{
	Stand(1, 5, 5, Flags::CELL_COLLISION_MODE_SOFT, "rifleman");
	Stand(2, 5, 5, Flags::CELL_COLLISION_MODE_SOFT, "grenadier");

	EXPECT_FALSE(Caught(Rules::Check(), "overlap", "stopped on it"));
}

TEST_F(RulesFixture, ABookingOverACellIsNotAStrandedStack)
{
	Alive(7, Flags::CELL_COLLISION_MODE_MEDIUM, "tank");
	Book(7, 6, 1, Flags::CELL_COLLISION_MODE_MEDIUM);

	EXPECT_TRUE(Rules::Check().empty());
}

TEST_F(RulesFixture, AUnitStandingOnItsOwnBookingIsAllowed)
{
	Stand(7, 6, 2, Flags::CELL_COLLISION_MODE_MEDIUM, "tank");
	Book(7, 6, 2, Flags::CELL_COLLISION_MODE_MEDIUM);

	EXPECT_TRUE(Rules::Check().empty());
}

TEST_F(RulesFixture, AStackNothingAccountsForIsCaught)
{
	Hold(1, 1, Flags::CELL_COLLISION_MODE_SOFT);

	EXPECT_TRUE(Caught(Rules::Check(), "stack", "with nothing standing on it"));
}

TEST_F(RulesFixture, AStackThatLostABodyIsCaught)
{
	Stand(1, 4, 1, Flags::CELL_COLLISION_MODE_MEDIUM, "tank");
	Hold(4, 1, Flags::CELL_COLLISION_MODE_MEDIUM * 2);

	EXPECT_TRUE(Caught(Rules::Check(), "stack", "come to " + std::to_string(Flags::CELL_COLLISION_MODE_MEDIUM)));
}

TEST_F(RulesFixture, AStackBelowNothingIsCaught)
{
	Hold(0, 7, -1);

	EXPECT_TRUE(Caught(Rules::Check(), "stack", "which no body can have put there"));
}

TEST_F(RulesFixture, AMarkTheStackDoesNotSupportIsCaught)
{
	Stand(1, 4, 4, Flags::CELL_COLLISION_MODE_MEDIUM, "tank");
	Mark(4, 4, Flags::CELL_COLLISION_MODE_OFF);

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
