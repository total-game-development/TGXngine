#pragma once

#include <gtest/gtest.h>
#include "Net/Lockstep.h"

namespace
{
using TGX::Net::Lockstep;
using TGX::Net::Tick;

// The buffer the clock keeps behind the server. Not exposed by Lockstep, so the
// tests work it out rather than restating it: whatever it is, the clock must not
// advance until the server is further ahead than that.
Tick MeasuredBuffer()
{
	Lockstep clock;

	clock.Begin(0);

	for (Tick ahead = 1; ahead < 1000; ahead++)
	{
		clock.SetServerTick(ahead);

		if (clock.ShouldAdvance())
		{
			return ahead - 1;
		}
	}

	return -1;
}

// Runs the clock the way the game scene does: drain what is due, then advance.
// Returns the ticks each command was applied at, in the order they came out.
TGX::Vector<Tick> Drain(Lockstep &clock, int budget = 4096)
{
	TGX::Vector<Tick> applied;

	while (clock.ShouldAdvance() && budget > 0)
	{
		budget--;

		for (const TGX::Net::Command &command : clock.Due())
		{
			applied.push_back(clock.LocalTick());
			(void)command;
		}

		clock.Advance();
	}

	return applied;
}
} // namespace

TEST(Lockstep, DoesNotAdvanceBeforeItIsStarted)
{
	Lockstep clock;

	clock.SetServerTick(1000);

	EXPECT_FALSE(clock.ShouldAdvance());
}

TEST(Lockstep, StaysAWholeBufferBehindTheServer)
{
	const Tick buffer = MeasuredBuffer();

	ASSERT_GT(buffer, 0) << "the clock never advanced, however far ahead the server was";

	Lockstep clock;

	clock.Begin(0);
	clock.SetServerTick(buffer);

	EXPECT_FALSE(clock.ShouldAdvance()) << "advanced with only the buffer between it and the server";

	clock.SetServerTick(buffer + 1);

	EXPECT_TRUE(clock.ShouldAdvance());
}

TEST(Lockstep, NeverSimulatesPastTheServer)
{
	Lockstep clock;

	clock.Begin(0);
	clock.SetServerTick(500);

	Drain(clock);

	EXPECT_LT(clock.LocalTick(), 500) << "ran to or past a tick the server has not broadcast";
}

TEST(Lockstep, BeginsWhereTheServerSaysTheMatchDoes)
{
	Lockstep clock;

	clock.Begin(900);

	EXPECT_EQ(clock.LocalTick(), 900);
	EXPECT_EQ(clock.ServerTick(), 900);
	EXPECT_EQ(clock.Lag(), 0);
}

TEST(Lockstep, HoldsACommandUntilTheTickItWasStampedFor)
{
	Lockstep clock;

	clock.Begin(0);
	clock.Accept(20, {1}, nlohmann::json::object());
	clock.SetServerTick(200);

	const TGX::Vector<Tick> applied = Drain(clock);

	ASSERT_EQ(applied.size(), 1u);
	EXPECT_EQ(applied.front(), 20);
}

TEST(Lockstep, AppliesACommandExactlyOnce)
{
	Lockstep clock;

	clock.Begin(0);
	clock.Accept(10, {1}, nlohmann::json::object());
	clock.SetServerTick(200);

	EXPECT_EQ(Drain(clock).size(), 1u);

	clock.SetServerTick(400);

	EXPECT_TRUE(Drain(clock).empty()) << "the same command came due a second time";
}

TEST(Lockstep, KeepsEveryCommandStampedForOneTick)
{
	Lockstep clock;

	clock.Begin(0);

	for (int index = 0; index < 5; index++)
	{
		clock.Accept(30, {index}, nlohmann::json::object());
	}

	clock.SetServerTick(200);

	const TGX::Vector<Tick> applied = Drain(clock);

	ASSERT_EQ(applied.size(), 5u);

	for (Tick at : applied)
	{
		EXPECT_EQ(at, 30);
	}
}

TEST(Lockstep, RefusesACommandStampedForATickItHasAlreadyRun)
{
	Lockstep clock;

	clock.Begin(0);
	clock.SetServerTick(200);

	Drain(clock);

	const Tick reached = clock.LocalTick();

	ASSERT_GT(reached, 0);

	clock.Accept(reached - 1, {1}, nlohmann::json::object());
	clock.SetServerTick(400);

	EXPECT_TRUE(Drain(clock).empty()) << "a command stamped in the past was applied";
}

TEST(Lockstep, OrdersCommandsByTheTickTheyWereStampedFor)
{
	Lockstep clock;

	clock.Begin(0);

	// Queued out of order, as they arrive when two clients act at once.
	clock.Accept(40, {1}, nlohmann::json::object());
	clock.Accept(20, {2}, nlohmann::json::object());
	clock.Accept(60, {3}, nlohmann::json::object());

	clock.SetServerTick(300);

	const TGX::Vector<Tick> applied = Drain(clock);

	ASSERT_EQ(applied.size(), 3u);
	EXPECT_EQ(applied[0], 20);
	EXPECT_EQ(applied[1], 40);
	EXPECT_EQ(applied[2], 60);
}

TEST(Lockstep, SaysWhenItIsCaughtUpAndWhenItIsReplaying)
{
	Lockstep clock;

	clock.Begin(0);
	clock.SetServerTick(4);

	EXPECT_TRUE(clock.IsCaughtUp());

	// What a client handed a match already thousands of ticks in looks like.
	clock.SetServerTick(5000);

	EXPECT_FALSE(clock.IsCaughtUp());

	Drain(clock, 8192);

	EXPECT_TRUE(clock.IsCaughtUp()) << "still reported as replaying after reaching the server";
}

TEST(Lockstep, ReplaysAMatchItJoinedPartWayThrough)
{
	Lockstep clock;

	clock.Begin(0);

	// The history a late joiner is handed: every command the match has stamped.
	for (Tick at = 0; at < 1200; at += 100)
	{
		clock.Accept(at, {static_cast<int>(at)}, nlohmann::json::object());
	}

	clock.SetServerTick(2000);

	const TGX::Vector<Tick> applied = Drain(clock);

	ASSERT_EQ(applied.size(), 12u);

	for (std::size_t index = 0; index < applied.size(); index++)
	{
		EXPECT_EQ(applied[index], static_cast<Tick>(index) * 100);
	}

	EXPECT_TRUE(clock.IsCaughtUp());
}

TEST(Lockstep, StoppingDropsWhatWasQueued)
{
	Lockstep clock;

	clock.Begin(0);
	clock.Accept(10, {1}, nlohmann::json::object());
	clock.Stop();

	EXPECT_FALSE(clock.IsRunning());
	EXPECT_FALSE(clock.ShouldAdvance());

	clock.Begin(0);
	clock.SetServerTick(200);

	EXPECT_TRUE(Drain(clock).empty()) << "a command survived into the next match";
}

TEST(Lockstep, IgnoresAServerTickThatGoesBackwards)
{
	Lockstep clock;

	clock.Begin(0);
	clock.SetServerTick(300);
	clock.SetServerTick(100);

	EXPECT_EQ(clock.ServerTick(), 300);
}
