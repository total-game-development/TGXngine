#pragma once

#include <gtest/gtest.h>
#include <limits>
#include "WorldState.h"

namespace
{
TGX::Vector<int> DrawInts(std::uint32_t seed, int minimum, int maximum, int count)
{
	TGX::WorldState &world = TGX::WorldState::GetInstance();

	world.SeedRandom(seed);

	TGX::Vector<int> drawn;
	drawn.reserve(static_cast<std::size_t>(count));

	for (int i = 0; i < count; i++)
	{
		drawn.push_back(world.RandomInt(minimum, maximum));
	}

	return drawn;
}
} // namespace

TEST(Random, SameSeedDrawsTheSameSequence)
{
	EXPECT_EQ(DrawInts(1234, 0, 99, 256), DrawInts(1234, 0, 99, 256));
}

TEST(Random, DifferentSeedsDiverge)
{
	EXPECT_NE(DrawInts(1234, 0, 99, 256), DrawInts(5678, 0, 99, 256));
}

TEST(Random, StaysWithinAnInclusiveRange)
{
	for (int value : DrawInts(99, -5, 5, 4096))
	{
		EXPECT_GE(value, -5);
		EXPECT_LE(value, 5);
	}
}

TEST(Random, ReachesBothEnds)
{
	const TGX::Vector<int> drawn = DrawInts(7, 0, 3, 4096);

	for (int wanted = 0; wanted <= 3; wanted++)
	{
		EXPECT_NE(std::ranges::find(drawn, wanted), drawn.end()) << "never drew " << wanted;
	}
}

TEST(Random, SpansTheWholeIntRangeWithoutOverflowing)
{
	TGX::WorldState &world = TGX::WorldState::GetInstance();

	world.SeedRandom(3);

	for (int i = 0; i < 512; i++)
	{
		const int value = world.RandomInt((std::numeric_limits<int>::min)(), (std::numeric_limits<int>::max)());

		EXPECT_GE(value, (std::numeric_limits<int>::min)());
		EXPECT_LE(value, (std::numeric_limits<int>::max)());
	}
}

TEST(Random, AnEmptyRangeIsItsOwnAnswer)
{
	TGX::WorldState &world = TGX::WorldState::GetInstance();

	world.SeedRandom(1);

	EXPECT_EQ(world.RandomInt(4, 4), 4);
	EXPECT_EQ(world.RandomInt(9, 2), 9);
}

TEST(Random, AnEmptyRangeDrawsNothingFromTheStream)
{
	TGX::WorldState &world = TGX::WorldState::GetInstance();

	world.SeedRandom(42);
	const int after = (world.RandomInt(4, 4), world.RandomInt(0, 1000));

	world.SeedRandom(42);
	const int alone = world.RandomInt(0, 1000);

	EXPECT_EQ(after, alone);
}

TEST(Random, FloatsStayInRangeAndRepeat)
{
	TGX::WorldState &world = TGX::WorldState::GetInstance();

	world.SeedRandom(11);

	TGX::Vector<float> first;
	for (int i = 0; i < 512; i++)
	{
		const float value = world.RandomFloat(-2.0f, 2.0f);

		EXPECT_GE(value, -2.0f);
		EXPECT_LT(value, 2.0f);

		first.push_back(value);
	}

	world.SeedRandom(11);

	for (float previous : first)
	{
		EXPECT_FLOAT_EQ(world.RandomFloat(-2.0f, 2.0f), previous);
	}
}
