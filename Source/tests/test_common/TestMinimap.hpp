#pragma once

#include <gtest/gtest.h>
#include "WorldState.h"

namespace
{
class MinimapBuilding : public TGX::ItemInstance
{
public:
	int GetFrames() const override
	{
		return 1;
	}

	float GetRadius() const override
	{
		return 9.0f;
	}

	int GetCellCollisionMode() const override
	{
		return 0;
	}

	void AddToGrid(TGX::Vector<TGX::Vector<int>> &grid, TGX::GridTracker &gridTracker) const override
	{
	}

	void RemoveFromGrid(TGX::Vector<TGX::Vector<int>> &grid, TGX::GridTracker &gridTracker) const override
	{
	}
};

class MinimapFixture : public ::testing::Test
{
protected:
	void SetUp() override
	{
		TGX::WorldState &world = TGX::WorldState::GetInstance();

		world.items.clear();

		for (const TGX::String &team : {"technology", "social-earth"})
		{
			world.SetPowerCut(team, false);
			world.SetPowerTotal(team, 400);
			world.SetPowerUsage(team, 100);
		}
	}

	void TearDown() override
	{
		TGX::WorldState::GetInstance().items.clear();
	}

	MinimapBuilding &Raise(const TGX::String &team, const TGX::String &name)
	{
		auto building = std::make_unique<MinimapBuilding>();

		building->SetTeam(team);
		building->SetName(name);
		building->SetLife(500.0f);

		MinimapBuilding &raised = *building;

		TGX::WorldState::GetInstance().items.push_back(std::move(building));

		return raised;
	}
};
} // namespace

TEST_F(MinimapFixture, WorksWithAPoweredRadar)
{
	Raise("technology", "radar");

	EXPECT_TRUE(TGX::WorldState::GetInstance().HasStanding("technology", "radar"));
	EXPECT_TRUE(TGX::WorldState::GetInstance().IsOperating("technology", "radar"));
}

TEST_F(MinimapFixture, IsDarkWithoutARadar)
{
	Raise("technology", "powerplant");

	EXPECT_FALSE(TGX::WorldState::GetInstance().HasStanding("technology", "radar"));
	EXPECT_FALSE(TGX::WorldState::GetInstance().IsOperating("technology", "radar"));
}

TEST_F(MinimapFixture, DoesNotBorrowTheOtherSidesRadar)
{
	Raise("social-earth", "radar");

	EXPECT_FALSE(TGX::WorldState::GetInstance().IsOperating("technology", "radar"));
}

TEST_F(MinimapFixture, GoesDarkWhenTheGridIsShort)
{
	Raise("technology", "radar");

	TGX::WorldState::GetInstance().SetPowerUsage("technology", 500);

	EXPECT_TRUE(TGX::WorldState::GetInstance().HasStanding("technology", "radar"));
	EXPECT_FALSE(TGX::WorldState::GetInstance().IsOperating("technology", "radar"));
}

TEST_F(MinimapFixture, GoesDarkWhenThePowerIsCut)
{
	Raise("technology", "radar");

	TGX::WorldState::GetInstance().SetPowerCut("technology", true);

	EXPECT_FALSE(TGX::WorldState::GetInstance().IsOperating("technology", "radar"));
}

TEST_F(MinimapFixture, GoesDarkWhenTheRadarIsSwitchedOff)
{
	Raise("technology", "radar").SetRunning(false);

	EXPECT_TRUE(TGX::WorldState::GetInstance().HasStanding("technology", "radar"));
	EXPECT_FALSE(TGX::WorldState::GetInstance().IsOperating("technology", "radar"));
}

TEST_F(MinimapFixture, GoesDarkWhenTheRadarIsDestroyed)
{
	Raise("technology", "radar").SetLife(0.0f);

	EXPECT_FALSE(TGX::WorldState::GetInstance().HasStanding("technology", "radar"));
	EXPECT_FALSE(TGX::WorldState::GetInstance().IsOperating("technology", "radar"));
}

TEST_F(MinimapFixture, ComesBackWhenAnyRadarStillWorks)
{
	Raise("technology", "radar").SetRunning(false);
	Raise("technology", "radar");

	EXPECT_TRUE(TGX::WorldState::GetInstance().IsOperating("technology", "radar"));
}
