#pragma once

#include <chrono>
#include <gtest/gtest.h>
#include <thread>
#include "Terminal.h"

namespace TGX::Shell
{
inline bool Printed(const Terminal &terminal, const String &text)
{
	for (const String &line : terminal.GetOutput())
	{
		if (line.find(text) != String::npos)
		{
			return true;
		}
	}

	return false;
}

inline bool WaitFor(const Function<bool()> &condition)
{
	for (int attempt = 0; attempt < 200; ++attempt)
	{
		if (condition())
		{
			return true;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	return false;
}

inline nlohmann::json Building(int uid, const String &name, int power)
{
	return {{"uid", uid}, {"name", name}, {"running", true}, {"power", power}};
}

struct FakeBase
{
	nlohmann::json listing = {
		{"usage", 30},
		{"total", 400},
		{"processes", nlohmann::json::array({
			Building(5, "construction_facility", 0),
			Building(9, "powerplant", -400),
			Building(12, "barracks", 30)})}};

	Vector<int> killed;

	void Attach(Terminal &terminal)
	{
		terminal.SetProcessHandlers(
			[this]() { return listing; },
			[this](int uid) {
				killed.push_back(uid);

				for (auto &entry : listing["processes"])
				{
					if (entry["uid"] == uid)
					{
						entry["running"] = false;
					}
				}

				return true;
			});
	}
};

TEST(ShellProcesses, ReportsNothingWhenNothingRuns)
{
	Terminal terminal;
	terminal.Submit("ps");

	EXPECT_TRUE(Printed(terminal, "No processes running"));
}

TEST(ShellProcesses, ListsBuildingsInTheOrderTheyWereBuilt)
{
	Terminal terminal;
	FakeBase base;
	base.Attach(terminal);

	terminal.Submit("ps");

	EXPECT_TRUE(Printed(terminal, "    1  running       0  construction_facility"));
	EXPECT_TRUE(Printed(terminal, "    2  running    +400  powerplant"));
	EXPECT_TRUE(Printed(terminal, "    3  running     -30  barracks"));
	EXPECT_TRUE(Printed(terminal, "Power 30 / 400"));
}

TEST(ShellProcesses, KillStopsABuilding)
{
	Terminal terminal;
	FakeBase base;
	base.Attach(terminal);

	terminal.Submit("ps");
	terminal.Submit("kill 2");

	ASSERT_EQ(base.killed.size(), 1u);
	EXPECT_EQ(base.killed[0], 9);
	EXPECT_TRUE(Printed(terminal, "Stopping powerplant (2)"));

	terminal.Submit("ps");
	EXPECT_TRUE(Printed(terminal, "    2  stopped       0  powerplant"));

	terminal.Submit("kill 2");
	EXPECT_TRUE(Printed(terminal, "Process 2 is already stopped"));
	EXPECT_EQ(base.killed.size(), 1u);
}

TEST(ShellProcesses, KillRejectsWhatIsNotAProcess)
{
	Terminal terminal;
	FakeBase base;
	base.Attach(terminal);

	terminal.Submit("kill 99");
	EXPECT_TRUE(Printed(terminal, "No such process 99"));

	terminal.Submit("kill two");
	EXPECT_TRUE(Printed(terminal, "Invalid pid two"));

	terminal.Submit("kill");
	EXPECT_TRUE(Printed(terminal, "Usage: kill <pid>"));

	EXPECT_TRUE(base.killed.empty());
}

TEST(ShellProcesses, ANewBuildingIsANewProcess)
{
	Terminal terminal;
	FakeBase base;
	base.Attach(terminal);

	terminal.Submit("ps");

	base.listing["processes"].push_back(Building(15, "powerplant", -400));
	base.listing["processes"].erase(2);

	terminal.Submit("clear");
	terminal.Submit("ps");

	EXPECT_TRUE(Printed(terminal, "    2  running    +400  powerplant"));
	EXPECT_TRUE(Printed(terminal, "    4  running    +400  powerplant"));
	EXPECT_FALSE(Printed(terminal, "barracks"));
}

TEST(ShellProcesses, KillInterruptsARunawayProgram)
{
	Terminal terminal;
	terminal.Start();

	terminal.WriteFile("loop", "let i = 0 while (1 == 1) { i = i + 1 }");
	terminal.Spawn("loop");

	ASSERT_TRUE(WaitFor([&]() {
		terminal.Submit("ps");
		return Printed(terminal, "    1  running       -  loop (program)");
	}));

	terminal.Submit("kill 1");
	EXPECT_TRUE(Printed(terminal, "Killed 1"));

	EXPECT_TRUE(WaitFor([&]() {
		terminal.Submit("clear");
		terminal.Submit("ps");
		return Printed(terminal, "No processes running");
	}));

	terminal.Stop();
}

TEST(ShellProcesses, StoppingTheShellEndsARunawayProgram)
{
	Terminal terminal;
	terminal.Start();

	terminal.WriteFile("loop", "let i = 0 while (1 == 1) { i = i + 1 }");
	terminal.Spawn("loop");

	ASSERT_TRUE(WaitFor([&]() {
		terminal.Submit("ps");
		return Printed(terminal, "loop (program)");
	}));

	terminal.Stop();

	terminal.Submit("clear");
	terminal.Submit("ps");
	EXPECT_TRUE(Printed(terminal, "No processes running"));
}
} // namespace TGX::Shell
