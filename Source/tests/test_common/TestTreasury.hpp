#pragma once

#include <gtest/gtest.h>
#include "WorldState.h"

namespace
{
TGX::WorldState &TreasuryFixture()
{
	TGX::WorldState &world = TGX::WorldState::GetInstance();

	world.economies.clear();

	for (const char *name : {"technology", "social-earth"})
	{
		auto economy = std::make_unique<TGX::EconomyInstance>();

		economy->SetTeam(name);
		economy->SetCash(1000);

		world.economies.push_back(std::move(economy));
	}

	world.SetTeam("technology");
	world.SetCash(1000);

	return world;
}
} // namespace

TEST(Treasury, FindsATeamsPurse)
{
	TGX::WorldState &world = TreasuryFixture();

	ASSERT_NE(world.FindEconomy("technology"), nullptr);
	ASSERT_NE(world.FindEconomy("social-earth"), nullptr);
	EXPECT_EQ(world.FindEconomy("nobody"), nullptr);
}

TEST(Treasury, ReadsZeroForATeamThatHasNoPurse)
{
	TGX::WorldState &world = TreasuryFixture();

	EXPECT_EQ(world.GetTeamCash("nobody"), 0);
}

TEST(Treasury, TakesTheCostOffTheTeamThatPaid)
{
	TGX::WorldState &world = TreasuryFixture();

	EXPECT_TRUE(world.SpendTeamCash("technology", 300));

	EXPECT_EQ(world.GetTeamCash("technology"), 700);
	EXPECT_EQ(world.GetTeamCash("social-earth"), 1000) << "the other side paid for it";
}

TEST(Treasury, RefusesWhatTheTeamCannotAfford)
{
	TGX::WorldState &world = TreasuryFixture();

	EXPECT_FALSE(world.SpendTeamCash("technology", 1001));
	EXPECT_EQ(world.GetTeamCash("technology"), 1000) << "a refused purchase still took the money";
}

TEST(Treasury, SpendsRightDownToNothing)
{
	TGX::WorldState &world = TreasuryFixture();

	EXPECT_TRUE(world.SpendTeamCash("technology", 1000));
	EXPECT_EQ(world.GetTeamCash("technology"), 0);

	EXPECT_FALSE(world.SpendTeamCash("technology", 1));
}

TEST(Treasury, RefusesToSpendForATeamThatHasNoPurse)
{
	TGX::WorldState &world = TreasuryFixture();

	EXPECT_FALSE(world.SpendTeamCash("nobody", 1));
}

// The number on screen follows the local player's purse. Another side spending
// is simulated on every client, and must not move the local readout.
TEST(Treasury, MirrorsOnlyTheLocalTeamOntoTheReadout)
{
	TGX::WorldState &world = TreasuryFixture();

	world.SpendTeamCash("social-earth", 400);

	EXPECT_EQ(world.GetCash(), 1000) << "the other side's spending moved the local readout";

	world.SpendTeamCash("technology", 250);

	EXPECT_EQ(world.GetCash(), 750);
}

TEST(Treasury, HandsResourceProgressBackInNameOrder)
{
	TGX::EconomyInstance economy;

	economy.AddResourceProgress("zinc", 3.0f);
	economy.AddResourceProgress("aluminium", 1.0f);
	economy.AddResourceProgress("magnesium", 2.0f);

	const auto ordered = economy.OrderedProgress();

	ASSERT_EQ(ordered.size(), 3u);
	EXPECT_EQ(ordered[0].first, "aluminium");
	EXPECT_EQ(ordered[1].first, "magnesium");
	EXPECT_EQ(ordered[2].first, "zinc");
}

// The container behind it is hashed, so the order it yields is not the order it
// was filled in. Two clients holding the same progress must still fold the same.
TEST(Treasury, OrdersProgressTheSameWhicheverWayItWasFilled)
{
	TGX::EconomyInstance forward;
	TGX::EconomyInstance backward;

	const TGX::Vector<TGX::String> names = {"ore", "crystal", "gas", "scrap", "alloy", "fuel"};

	for (const TGX::String &name : names)
	{
		forward.AddResourceProgress(name, 1.0f);
	}

	for (auto entry = names.rbegin(); entry != names.rend(); ++entry)
	{
		backward.AddResourceProgress(*entry, 1.0f);
	}

	EXPECT_EQ(forward.OrderedProgress(), backward.OrderedProgress());
}
