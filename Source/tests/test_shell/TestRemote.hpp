#pragma once

#include <gtest/gtest.h>
#include "Terminal.h"
#include "TestProcesses.hpp"

namespace TGX::Shell
{
struct Loopback
{
	Map<String, Terminal *> terminals;
	Vector<Pair<String, nlohmann::json>> inflight;

	void Join(Terminal &terminal, const String &name, const Vector<String> &peers)
	{
		terminals[name] = &terminal;

		terminal.SetNetwork(name, peers, [this, name](const String &to, const nlohmann::json &body) {
			inflight.push_back({to, {{"type", "shell"}, {"from", name}, {"body", body}}});
		});
	}

	void Pump()
	{
		while (!inflight.empty())
		{
			Vector<Pair<String, nlohmann::json>> batch;
			batch.swap(inflight);

			for (const auto &[to, message] : batch)
			{
				const auto found = terminals.find(to);

				if (found != terminals.end())
				{
					found->second->Deliver(message);
				}
			}
		}
	}
};

struct RemoteFixture : ::testing::Test
{
	Terminal ruby;
	Terminal sapphire;
	Loopback network;

	void SetUp() override
	{
		network.Join(ruby, "ruby", {"ruby", "sapphire"});
		network.Join(sapphire, "sapphire", {"ruby", "sapphire"});

		ruby.Submit("passwd 1111");
		sapphire.Submit("passwd 2222");
	}

	void Run(Terminal &terminal, const String &command)
	{
		terminal.Submit(command);
		network.Pump();
	}
};

TEST_F(RemoteFixture, ListsTheOtherPlayers)
{
	Run(ruby, "hosts");

	EXPECT_TRUE(Printed(ruby, " - sapphire"));
	EXPECT_FALSE(Printed(ruby, " - ruby"));
}

TEST_F(RemoteFixture, ConnectsToAnotherPlayersComputer)
{
	Run(ruby, "connect sapphire 2222");

	EXPECT_TRUE(Printed(ruby, "Connected to sapphire"));
	EXPECT_TRUE(Printed(sapphire, "ruby connected to this computer"));
	EXPECT_NE(ruby.GetPrompt().find("@sapphire:/"), String::npos);
}

TEST_F(RemoteFixture, WorksOnTheRemoteFilesystemNotTheLocalOne)
{
	Run(sapphire, "mkdir docs");
	Run(ruby, "connect sapphire 2222");

	Run(ruby, "ls");
	EXPECT_TRUE(Printed(ruby, "docs/"));

	Run(ruby, "cd docs");
	EXPECT_NE(ruby.GetPrompt().find("@sapphire:/docs/"), String::npos);
	EXPECT_NE(sapphire.GetPrompt().find(":/>"), String::npos);

	Run(ruby, "mk notes");
	EXPECT_TRUE(Printed(ruby, "File notes created"));

	Run(ruby, "rn notes plans");

	Run(sapphire, "cd docs");

	String source;
	EXPECT_TRUE(sapphire.ReadFile("plans", source));
	EXPECT_FALSE(sapphire.ReadFile("notes", source));

	Run(ruby, "disconnect");
	Run(ruby, "ls");
	EXPECT_FALSE(ruby.ReadFile("plans", source));
}

TEST_F(RemoteFixture, EditsARemoteFile)
{
	sapphire.WriteFile("hello", "print(1)");

	Run(ruby, "connect sapphire 2222");
	Run(ruby, "edit hello");

	ASSERT_EQ(ruby.GetMode(), TerminalMode::Editing);
	EXPECT_EQ(ruby.GetEditor().Source(), "print(1)");

	ruby.GetEditor().Insert('#');
	const String edited = ruby.GetEditor().Source();

	ruby.Escape();
	network.Pump();

	EXPECT_EQ(ruby.GetMode(), TerminalMode::Command);
	EXPECT_TRUE(Printed(ruby, "Saved hello"));

	String source;
	EXPECT_TRUE(sapphire.ReadFile("hello", source));
	EXPECT_EQ(source, edited);
}

TEST_F(RemoteFixture, ReportsAMissingRemoteFile)
{
	Run(ruby, "connect sapphire 2222");
	Run(ruby, "edit nothing");

	EXPECT_EQ(ruby.GetMode(), TerminalMode::Command);
	EXPECT_TRUE(Printed(ruby, "File nothing doesn't exist"));
}

TEST_F(RemoteFixture, RunsAProgramOnTheComputerItIsConnectedTo)
{
	sapphire.WriteFile("hello", "print(\"from sapphire\")");

	Run(ruby, "connect sapphire 2222");
	Run(ruby, "run hello");

	EXPECT_TRUE(Printed(ruby, "from sapphire"));
	EXPECT_TRUE(Printed(sapphire, "ruby ran hello on this computer"));
	EXPECT_TRUE(Printed(sapphire, "from sapphire"));
}

TEST_F(RemoteFixture, TheShorthandRunsThereToo)
{
	sapphire.WriteFile("hello", "print(\"from sapphire\")");

	Run(ruby, "connect sapphire 2222");
	Run(ruby, "./hello");

	EXPECT_TRUE(Printed(ruby, "from sapphire"));
}

TEST_F(RemoteFixture, AProgramReadsTheFilesOfTheComputerItRunsOn)
{
	sapphire.WriteFile("secret", "orders");
	sapphire.WriteFile("peek", "print(read(\"secret\"))");
	ruby.WriteFile("secret", "mine");

	Run(ruby, "connect sapphire 2222");
	Run(ruby, "run peek");

	EXPECT_TRUE(Printed(ruby, "orders"));
	EXPECT_FALSE(Printed(ruby, "mine"));
}

TEST_F(RemoteFixture, AMissingProgramIsReportedBack)
{
	Run(ruby, "connect sapphire 2222");
	Run(ruby, "run nothing");

	EXPECT_TRUE(Printed(ruby, "File nothing doesn't exist"));
}

TEST_F(RemoteFixture, AProgramCannotToggleTheComputerItBrokeInto)
{
	sapphire.WriteFile("flip", "toggle(\"fogofwar\", \"show\", 1)");

	Run(ruby, "connect sapphire 2222");
	Run(ruby, "run flip");

	EXPECT_TRUE(Printed(sapphire, "Toggles are refused from a remote session"));
	EXPECT_FALSE(Printed(sapphire, "Unknown toggle"));
}

TEST_F(RemoteFixture, RunningNeedsASessionLikeAnythingElse)
{
	sapphire.WriteFile("hello", "print(\"from sapphire\")");

	Run(ruby, "connect sapphire 9999");
	Run(ruby, "run hello");

	EXPECT_FALSE(Printed(ruby, "from sapphire"));
	EXPECT_FALSE(Printed(sapphire, "ruby ran hello on this computer"));
}

TEST_F(RemoteFixture, ListsAndStopsProcessesOnTheComputerItBrokeInto)
{
	FakeBase base;
	base.Attach(sapphire);

	Run(ruby, "connect sapphire 2222");
	Run(ruby, "ps");

	EXPECT_TRUE(Printed(ruby, "powerplant"));
	EXPECT_TRUE(Printed(ruby, "Power 30 / 400"));

	Run(ruby, "kill 2");

	EXPECT_TRUE(Printed(ruby, "Stopping powerplant (2)"));
	EXPECT_TRUE(Printed(sapphire, "ruby sent kill 2 to this computer"));
	ASSERT_EQ(base.killed.size(), 1u);
	EXPECT_EQ(base.killed.front(), 9);
}

TEST_F(RemoteFixture, ProcessesAreStillYourOwnWhenNobodyIsConnected)
{
	FakeBase base;
	base.Attach(ruby);

	Run(ruby, "ps");

	EXPECT_TRUE(Printed(ruby, "powerplant"));
}

TEST_F(RemoteFixture, ADirectoryRemovedUnderneathIsReported)
{
	Run(sapphire, "mkdir docs");
	Run(ruby, "connect sapphire 2222");
	Run(ruby, "cd docs");
	Run(sapphire, "del docs");
	Run(ruby, "ls");

	EXPECT_TRUE(Printed(ruby, "Directory /docs/ doesn't exist"));
}

TEST_F(RemoteFixture, DisconnectingTellsTheOtherComputer)
{
	Run(ruby, "connect sapphire 2222");
	Run(ruby, "disconnect");

	EXPECT_TRUE(Printed(sapphire, "ruby disconnected from this computer"));
	EXPECT_NE(ruby.GetPrompt().find("@local:"), String::npos);
}

TEST_F(RemoteFixture, ProgramsKeepTheirOwnFilesWhileConnected)
{
	ruby.WriteFile("mine", "1");

	Run(ruby, "connect sapphire 2222");

	String source;
	EXPECT_TRUE(ruby.ReadFile("mine", source));
	EXPECT_EQ(source, "1");
	EXPECT_FALSE(sapphire.ReadFile("mine", source));
}

TEST_F(RemoteFixture, CheatsAreDisabledInMultiplayer)
{
	Run(ruby, "cheat when the walls fell");

	EXPECT_TRUE(Printed(ruby, "Cheats are disabled in multiplayer"));
}

struct Grid
{
	bool cut = false;
	int asked = 0;

	void Attach(Terminal &terminal)
	{
		terminal.SetHackHandler([this](const String &effect, bool wanted) {
			asked++;

			if (effect != "power" || cut == wanted)
			{
				return false;
			}

			cut = wanted;

			return true;
		});
	}
};

TEST_F(RemoteFixture, CutsTheGridOfTheComputerItBrokeInto)
{
	Grid grid;
	grid.Attach(sapphire);

	Run(ruby, "connect sapphire 2222");
	Run(ruby, "hack power");

	EXPECT_TRUE(grid.cut);
	EXPECT_TRUE(Printed(sapphire, "ruby cut the power on this computer"));
	EXPECT_TRUE(Printed(ruby, "The grid on sapphire is cut"));
}

TEST_F(RemoteFixture, AGridAlreadyCutIsNotCutTwice)
{
	Grid grid;
	grid.cut = true;
	grid.Attach(sapphire);

	Run(ruby, "connect sapphire 2222");
	Run(ruby, "hack power");

	EXPECT_TRUE(Printed(ruby, "The grid on sapphire is already cut"));
}

TEST_F(RemoteFixture, AHackNeedsASessionLikeAnythingElse)
{
	Grid grid;
	grid.Attach(sapphire);

	Run(ruby, "connect sapphire 9999");
	Run(ruby, "hack power");

	EXPECT_EQ(grid.asked, 0);
	EXPECT_FALSE(grid.cut);
}

TEST_F(RemoteFixture, ThereIsNothingToHackFromYourOwnConsole)
{
	Grid grid;
	grid.Attach(ruby);

	Run(ruby, "hack power");

	EXPECT_EQ(grid.asked, 0);
	EXPECT_TRUE(Printed(ruby, "There is nothing to hack from your own console"));
}

TEST_F(RemoteFixture, TheOwnerPutsItsOwnGridBack)
{
	Grid grid;
	grid.cut = true;
	grid.Attach(sapphire);

	Run(sapphire, "restore");

	EXPECT_FALSE(grid.cut);
	EXPECT_TRUE(Printed(sapphire, "Restoring the grid"));
}

TEST_F(RemoteFixture, AGridNobodyCutNeedsNoRestoring)
{
	Grid grid;
	grid.Attach(sapphire);

	Run(sapphire, "restore");

	EXPECT_TRUE(Printed(sapphire, "Your grid has not been cut"));
}

TEST_F(RemoteFixture, SomebodyElsesGridIsNotYoursToRestore)
{
	Grid grid;
	grid.cut = true;
	grid.Attach(sapphire);

	Run(ruby, "connect sapphire 2222");
	Run(ruby, "restore");

	EXPECT_TRUE(grid.cut);
	EXPECT_TRUE(Printed(ruby, "The grid on sapphire is not yours to restore"));
}

TEST_F(RemoteFixture, ACutGridIsSaidSoInPs)
{
	FakeBase base;
	base.listing["cut"] = true;
	base.Attach(sapphire);

	Run(ruby, "connect sapphire 2222");
	Run(ruby, "ps");

	EXPECT_TRUE(Printed(ruby, "Power 30 / 400 (cut)"));
}

TEST(ShellRemote, AnUnreachableComputerDropsBackToLocal)
{
	Terminal terminal;
	terminal.SetNetwork("ruby", {"sapphire"}, [](const String &, const nlohmann::json &) {});

	terminal.Submit("connect sapphire 2222");
	terminal.Deliver({{"type", "shell_refused"}, {"to", "sapphire"}, {"id", 1}, {"reason", "is not reachable"}});

	EXPECT_TRUE(Printed(terminal, "Computer sapphire is not reachable"));
	EXPECT_NE(terminal.GetPrompt().find("@local:"), String::npos);
}

TEST(ShellRemote, AComputerOffTheNetworkIsNotFound)
{
	Terminal terminal;

	terminal.Submit("hosts");
	terminal.Submit("connect sapphire");

	EXPECT_TRUE(Printed(terminal, "Not on a network"));
	EXPECT_TRUE(Printed(terminal, "Usage: connect <computer>"));
}

TEST_F(RemoteFixture, RefusesAConnectionWithoutThePin)
{
	Run(ruby, "connect sapphire");

	EXPECT_TRUE(Printed(ruby, "Usage: connect <computer> <pin>"));
	EXPECT_FALSE(Printed(sapphire, "ruby connected to this computer"));
	EXPECT_NE(ruby.GetPrompt().find("@local:"), String::npos);
}

TEST_F(RemoteFixture, RefusesAConnectionWithTheWrongPin)
{
	Run(ruby, "connect sapphire 9999");

	EXPECT_TRUE(Printed(ruby, "Computer sapphire denied access"));
	EXPECT_TRUE(Printed(sapphire, "ruby was refused a connection to this computer"));
	EXPECT_FALSE(Printed(sapphire, "ruby connected to this computer"));
	EXPECT_NE(ruby.GetPrompt().find("@local:"), String::npos);
}

TEST_F(RemoteFixture, AnUnauthorisedRequestTouchesNothing)
{
	Run(sapphire, "mkdir docs");

	sapphire.Deliver({{"type", "shell"},
					  {"from", "ruby"},
					  {"body", {{"kind", "request"}, {"id", 7}, {"op", "del"}, {"cwd", "/"}, {"args", {"docs"}}}}});

	network.Pump();

	Run(sapphire, "ls");
	EXPECT_TRUE(Printed(sapphire, "docs/"));
}

TEST_F(RemoteFixture, ChangingThePinShutsOutWhoeverIsConnected)
{
	Run(ruby, "connect sapphire 2222");
	Run(ruby, "ls");

	Run(sapphire, "passwd 3333");
	EXPECT_TRUE(Printed(sapphire, "Pin changed"));

	Run(ruby, "mkdir docs");
	EXPECT_TRUE(Printed(ruby, "Access denied"));

	Run(ruby, "disconnect");
	Run(ruby, "connect sapphire 3333");
	EXPECT_TRUE(Printed(ruby, "Connected to sapphire"));
}

TEST_F(RemoteFixture, SaysWhoIsConnected)
{
	Run(sapphire, "who");
	EXPECT_TRUE(Printed(sapphire, "This computer's pin is 2222"));
	EXPECT_TRUE(Printed(sapphire, "Nobody is connected to it"));

	Run(ruby, "connect sapphire 2222");
	Run(sapphire, "who");

	EXPECT_TRUE(Printed(sapphire, " - ruby is connected"));

	Run(ruby, "disconnect");
	Run(sapphire, "who");

	EXPECT_TRUE(Printed(sapphire, "Nobody is connected to it"));
}

TEST_F(RemoteFixture, ThePinIsChangedFromItsOwnConsoleOnly)
{
	Run(ruby, "connect sapphire 2222");
	Run(ruby, "passwd 4444");

	EXPECT_TRUE(Printed(ruby, "The pin on sapphire can only be changed from its own console"));

	Run(ruby, "disconnect");
	Run(ruby, "connect sapphire 2222");

	EXPECT_TRUE(Printed(ruby, "Connected to sapphire"));
}

TEST_F(RemoteFixture, APinIsDigitsOnly)
{
	Run(ruby, "passwd letmein");

	EXPECT_TRUE(Printed(ruby, "A pin is digits only"));
}

TEST(ShellAccess, EveryComputerStartsWithAPinOfItsOwn)
{
	const auto pin = [](const Terminal &terminal) {
		for (const String &line : terminal.GetOutput())
		{
			const std::size_t at = line.find("pin is ");

			if (at != String::npos)
			{
				return line.substr(at + 7);
			}
		}

		return String();
	};

	Set<String> seen;

	for (int index = 0; index < 8; index++)
	{
		Terminal terminal;
		terminal.Submit("who");

		const String value = pin(terminal);

		EXPECT_EQ(value.size(), 4u);
		EXPECT_EQ(value.find_first_not_of("0123456789"), String::npos);

		seen.insert(value);
	}

	EXPECT_GT(seen.size(), 1u);
}

TEST(ShellRemote, CheatsStillWorkOffline)
{
	Terminal terminal;

	terminal.Submit("cheat when the walls fell");

	EXPECT_FALSE(Printed(terminal, "Cheats are disabled"));
	EXPECT_TRUE(Printed(terminal, "Unknown toggle: fogofwar show"));
}
} // namespace TGX::Shell
