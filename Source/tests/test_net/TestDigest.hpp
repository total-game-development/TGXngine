#pragma once

#include <cstring>
#include <gtest/gtest.h>
#include "Net/Digest.h"

namespace
{
using TGX::Net::Digest;

// The server's fold, written out again rather than shared, because the two are
// separate programs in separate repositories and the whole point of the check is
// that they agree. Transcribed from TGXngineServer/Source/Simulation.h: if that
// file changes, this fails and says so, which is the only warning there will be
// before every sanity check in a live match reads as a desync.
class ServerFold
{
private:
	std::uint64_t fold = 0xCBF29CE484222325ULL;

public:
	void Mix(std::uint64_t value)
	{
		fold ^= value;
		fold *= 0x100000001B3ULL;
	}

	void MixText(const TGX::String &text)
	{
		for (unsigned char letter : text)
		{
			Mix(letter);
		}
	}

	std::uint64_t Value() const
	{
		return fold;
	}
};
} // namespace

TEST(Digest, StartsFromTheSameOffsetTheServerDoes)
{
	Digest fold;
	ServerFold reference;

	EXPECT_EQ(fold.Value(), reference.Value());
}

TEST(Digest, FoldsANumberTheWayTheServerDoes)
{
	Digest fold;
	ServerFold reference;

	for (std::uint64_t value : {0ULL, 1ULL, 42ULL, 0xFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL})
	{
		fold.Mix(value);
		reference.Mix(value);
	}

	EXPECT_EQ(fold.Value(), reference.Value());
}

TEST(Digest, FoldsTextTheWayTheServerDoes)
{
	Digest fold;
	ServerFold reference;

	// Includes bytes above 127, which fold differently if either side lets char
	// sign-extend on the way into the mix.
	const TGX::String text = "skirmish-plains \xC3\xA9 {\"kind\":\"order\",\"toX\":12.5}";

	fold.MixText(text);
	reference.MixText(text);

	EXPECT_EQ(fold.Value(), reference.Value());
}

// What ApplyCommand and TickOnlySimulation::Apply each do for one command. They
// are written in different repositories against the same description, so the
// test walks the whole sequence rather than the pieces.
TEST(Digest, FoldsAWholeCommandTheWayTheServerDoes)
{
	const std::uint32_t seed = 0xDEADBEEF;
	const TGX::String level = "skirmish-plains";
	const TGX::String orders = R"({"enemy":true,"kind":"order","order":3,"toX":128.0,"toY":64.0})";
	const std::int64_t tick = 240;
	const TGX::Vector<int> uids = {7, 11, 13};

	Digest fold;

	fold.Mix(seed);
	fold.MixText(level);
	fold.Mix(static_cast<std::uint64_t>(tick));

	for (int uid : uids)
	{
		fold.Mix(static_cast<std::uint64_t>(uid));
	}

	fold.MixText(orders);

	ServerFold reference;

	reference.Mix(seed);
	reference.MixText(level);
	reference.Mix(static_cast<std::uint64_t>(tick));

	for (int uid : uids)
	{
		reference.Mix(static_cast<std::uint64_t>(uid));
	}

	reference.MixText(orders);

	EXPECT_EQ(fold.Value(), reference.Value());
}

TEST(Digest, TellsTwoDifferentCommandSetsApart)
{
	Digest first;
	Digest second;

	first.Mix(10);
	first.MixText("move");

	second.Mix(10);
	second.MixText("attack");

	EXPECT_NE(first.Value(), second.Value());
}

TEST(Digest, TellsTheSameCommandsAtDifferentTicksApart)
{
	Digest first;
	Digest second;

	first.Mix(10);
	first.MixText("move");

	second.Mix(11);
	second.MixText("move");

	EXPECT_NE(first.Value(), second.Value());
}

TEST(Digest, FoldsAFloatByItsBitsSoTheSmallestDriftShows)
{
	Digest same;
	Digest drifted;

	same.MixFloat(128.0f);

	// One unit in the last place: the difference two clients would show after a
	// few thousand ticks of the same arithmetic in a different order.
	float nudged = 128.0f;
	std::uint32_t bits = 0;

	std::memcpy(&bits, &nudged, sizeof(bits));
	bits += 1;
	std::memcpy(&nudged, &bits, sizeof(bits));

	drifted.MixFloat(nudged);

	EXPECT_NE(same.Value(), drifted.Value());
}

TEST(Digest, FoldsTheSameFloatToTheSameNumber)
{
	Digest first;
	Digest second;

	for (float value : {0.0f, -0.0f, 1.5f, -273.15f, 1e-20f, 1e20f})
	{
		first.MixFloat(value);
		second.MixFloat(value);
	}

	EXPECT_EQ(first.Value(), second.Value());
}

TEST(Digest, ResetReturnsItToTheOffset)
{
	Digest fold;

	const std::uint64_t offset = fold.Value();

	fold.Mix(99);
	fold.MixText("something");

	ASSERT_NE(fold.Value(), offset);

	fold.Reset();

	EXPECT_EQ(fold.Value(), offset);
}

TEST(Digest, IsSensitiveToTheOrderThingsAreFoldedIn)
{
	Digest forward;
	Digest backward;

	forward.Mix(1);
	forward.Mix(2);

	backward.Mix(2);
	backward.Mix(1);

	EXPECT_NE(forward.Value(), backward.Value());
}
