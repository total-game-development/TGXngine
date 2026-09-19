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
	Run(ruby, "connect sapphire");

	EXPECT_TRUE(Printed(ruby, "Connected to sapphire"));
	EXPECT_TRUE(Printed(sapphire, "ruby connected to this computer"));
	EXPECT_NE(ruby.GetPrompt().find("@sapphire:/"), String::npos);
}

TEST_F(RemoteFixture, WorksOnTheRemoteFilesystemNotTheLocalOne)
{
	Run(sapphire, "mkdir docs");
	Run(ruby, "connect sapphire");

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

	Run(ruby, "connect sapphire");
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
	Run(ruby, "connect sapphire");
	Run(ruby, "edit nothing");

	EXPECT_EQ(ruby.GetMode(), TerminalMode::Command);
	EXPECT_TRUE(Printed(ruby, "File nothing doesn't exist"));
}

TEST_F(RemoteFixture, DoesNotRunProgramsOnARemoteComputer)
{
	sapphire.WriteFile("hello", "print(\"from sapphire\")");

	Run(ruby, "connect sapphire");
	Run(ruby, "run hello");
	Run(ruby, "./hello");

	EXPECT_TRUE(Printed(ruby, "Programs on sapphire can only be run from its own console"));
	EXPECT_FALSE(Printed(ruby, "from sapphire"));
	EXPECT_FALSE(Printed(sapphire, "from sapphire"));
}

TEST_F(RemoteFixture, KeepsProcessesToTheirOwnConsole)
{
	Run(ruby, "connect sapphire");
	Run(ruby, "ps");

	EXPECT_TRUE(Printed(ruby, "Processes on sapphire can only be seen from its own console"));
}

TEST_F(RemoteFixture, ADirectoryRemovedUnderneathIsReported)
{
	Run(sapphire, "mkdir docs");
	Run(ruby, "connect sapphire");
	Run(ruby, "cd docs");
	Run(sapphire, "del docs");
	Run(ruby, "ls");

	EXPECT_TRUE(Printed(ruby, "Directory /docs/ doesn't exist"));
}

TEST_F(RemoteFixture, DisconnectingTellsTheOtherComputer)
{
	Run(ruby, "connect sapphire");
	Run(ruby, "disconnect");

	EXPECT_TRUE(Printed(sapphire, "ruby disconnected from this computer"));
	EXPECT_NE(ruby.GetPrompt().find("@local:"), String::npos);
}

TEST_F(RemoteFixture, ProgramsKeepTheirOwnFilesWhileConnected)
{
	ruby.WriteFile("mine", "1");

	Run(ruby, "connect sapphire");

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

TEST(ShellRemote, AnUnreachableComputerDropsBackToLocal)
{
	Terminal terminal;
	terminal.SetNetwork("ruby", {"sapphire"}, [](const String &, const nlohmann::json &) {});

	terminal.Submit("connect sapphire");
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

TEST(ShellRemote, CheatsStillWorkOffline)
{
	Terminal terminal;

	terminal.Submit("cheat when the walls fell");

	EXPECT_FALSE(Printed(terminal, "Cheats are disabled"));
	EXPECT_TRUE(Printed(terminal, "Unknown toggle: fogofwar show"));
}
} // namespace TGX::Shell
