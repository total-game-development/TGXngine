#pragma once

#include <gtest/gtest.h>

#include "Cells.h"
#include "Flags.h"

namespace
{
using namespace TGX;

class SettleFixture : public ::testing::Test
{
protected:
	static constexpr int SIDE = 12;
	static constexpr float RADIUS = 0.4f;

	Vector<Vector<int>> grid;
	Map<int, TacticalReservation> bookings;
	Map<String, int> cells;

	void SetUp() override
	{
		grid.assign(SIDE, Vector<int>(SIDE, Flags::CELL_COLLISION_MODE_OFF));
		bookings.clear();
		cells.clear();
	}

	void Hold(int x, int y, int stack)
	{
		cells[std::to_string(x) + " " + std::to_string(y)] = stack;

		grid[y][x] = stack >= Flags::CELL_COLLISION_MODE_MEDIUM ? Flags::CELL_COLLISION_MODE_MEDIUM
					 : stack > Flags::CELL_COLLISION_MODE_OFF	? Flags::CELL_COLLISION_MODE_SOFT
																: Flags::CELL_COLLISION_MODE_OFF;
	}

	Pair<float, float> Settle(int uid, int x, int y, int mode)
	{
		return Cells::Settle(
			uid, static_cast<float>(x), static_cast<float>(y), RADIUS, mode, grid, bookings, cells);
	}
};

TEST_F(SettleFixture, GroundNobodyHoldsIsKept)
{
	const Pair<float, float> rest = Settle(1, 5, 5, Flags::CELL_COLLISION_MODE_MEDIUM);

	EXPECT_FLOAT_EQ(rest.first, 5.0f);
	EXPECT_FLOAT_EQ(rest.second, 5.0f);
}

TEST_F(SettleFixture, AVehicleStoppingOnAVehicleIsMovedOff)
{
	Hold(5, 5, Flags::CELL_COLLISION_MODE_MEDIUM);

	const Pair<float, float> rest = Settle(1, 5, 5, Flags::CELL_COLLISION_MODE_MEDIUM);

	EXPECT_FALSE(rest.first == 5.0f && rest.second == 5.0f);
}

TEST_F(SettleFixture, InfantryStoppingOnAVehicleIsMovedOff)
{
	Hold(5, 5, Flags::CELL_COLLISION_MODE_MEDIUM);

	const Pair<float, float> rest = Settle(1, 5, 5, Flags::CELL_COLLISION_MODE_SOFT);

	EXPECT_FALSE(rest.first == 5.0f && rest.second == 5.0f);
}

TEST_F(SettleFixture, InfantryStoppingOnInfantryStaysPut)
{
	Hold(5, 5, Flags::CELL_COLLISION_MODE_SOFT);

	const Pair<float, float> rest = Settle(1, 5, 5, Flags::CELL_COLLISION_MODE_SOFT);

	EXPECT_FLOAT_EQ(rest.first, 5.0f);
	EXPECT_FLOAT_EQ(rest.second, 5.0f);
}

TEST_F(SettleFixture, AVehicleStoppingOnInfantryIsMovedOff)
{
	Hold(5, 5, Flags::CELL_COLLISION_MODE_SOFT);

	const Pair<float, float> rest = Settle(1, 5, 5, Flags::CELL_COLLISION_MODE_MEDIUM);

	EXPECT_FALSE(rest.first == 5.0f && rest.second == 5.0f);
}

TEST_F(SettleFixture, AUnitIsNotPushedOffItsOwnBooking)
{
	Hold(5, 5, Flags::CELL_COLLISION_MODE_MEDIUM);

	bookings[1] = {5, 5, 5, 5, Flags::CELL_COLLISION_MODE_MEDIUM};

	const Pair<float, float> rest = Settle(1, 5, 5, Flags::CELL_COLLISION_MODE_MEDIUM);

	EXPECT_FLOAT_EQ(rest.first, 5.0f);
	EXPECT_FLOAT_EQ(rest.second, 5.0f);
}

TEST_F(SettleFixture, GroundIsNotTakenInsideAWall)
{
	Hold(5, 5, Flags::CELL_COLLISION_MODE_MEDIUM);

	for (int x = 4; x <= 6; x++)
	{
		for (int y = 4; y <= 6; y++)
		{
			if (x != 5 || y != 5)
			{
				grid[y][x] = Flags::CELL_COLLISION_MODE_HARD;
			}
		}
	}

	const Pair<float, float> rest = Settle(1, 5, 5, Flags::CELL_COLLISION_MODE_MEDIUM);

	EXPECT_GT(std::abs(rest.first - 5.0f) + std::abs(rest.second - 5.0f), 1.0f);
}

TEST_F(SettleFixture, AUnitWithNowhereToGoKeepsWhereItIs)
{
	for (int x = 0; x < SIDE; x++)
	{
		for (int y = 0; y < SIDE; y++)
		{
			Hold(x, y, Flags::CELL_COLLISION_MODE_MEDIUM);
		}
	}

	const Pair<float, float> rest = Settle(1, 5, 5, Flags::CELL_COLLISION_MODE_MEDIUM);

	EXPECT_FLOAT_EQ(rest.first, 5.0f);
	EXPECT_FLOAT_EQ(rest.second, 5.0f);
}
} // namespace
