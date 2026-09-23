#pragma once

#include <gtest/gtest.h>
#include "Terminal.h"
#include "TestProcesses.hpp"
#include "TestRemote.hpp"

namespace TGX::Shell
{
TEST_F(RemoteFixture, RefusesCyberCommandsOnAMapThatDoesNotAllowThem)
{
	ruby.SetCyber(false);

	for (const String &command : {"connect sapphire 2222", "hack power", "get radar", "rekey", "passwd 9999", "role cyber red"})
	{

		Run(ruby, command);
		EXPECT_EQ(ruby.GetOutput().back(), "Cyber commands are not allowed on this map") << command;
	}

	EXPECT_EQ(ruby.GetTint(), TerminalTint::None);
	EXPECT_FALSE(Printed(sapphire, "ruby connected to this computer"));
}

TEST_F(RemoteFixture, KeepsOrdinaryCommandsOnAMapThatDoesNotAllowCyber)
{
	ruby.SetCyber(false);

	Run(ruby, "mkdir work");
	Run(ruby, "ls");

	EXPECT_TRUE(Printed(ruby, "work/"));
}

TEST_F(RemoteFixture, DropsASessionWhenCyberIsTakenAway)
{
	Run(ruby, "connect sapphire 2222");
	ASSERT_NE(ruby.GetPrompt().find("@sapphire:/"), String::npos);

	ruby.SetCyber(false);

	EXPECT_EQ(ruby.GetPrompt().find("@sapphire:/"), String::npos);
}

TEST_F(RemoteFixture, LearnsOfViewersArrivingAndLeaving)
{
	ruby.Deliver({{"type", "consoles"}, {"consoles", {"ruby", "sapphire", "emerald"}}});

	Run(ruby, "hosts");
	EXPECT_TRUE(Printed(ruby, " - emerald"));

	Run(ruby, "connect sapphire 2222");
	ASSERT_NE(ruby.GetPrompt().find("@sapphire:/"), String::npos);

	ruby.Deliver({{"type", "consoles"}, {"consoles", {"ruby", "emerald"}}});

	EXPECT_TRUE(Printed(ruby, "sapphire left the network"));
	EXPECT_EQ(ruby.GetPrompt().find("@sapphire:/"), String::npos);

	Run(ruby, "clear");
	Run(ruby, "hosts");
	EXPECT_TRUE(Printed(ruby, " - emerald"));
	EXPECT_FALSE(Printed(ruby, " - sapphire"));
}

TEST(ShellCyber, LeavesATutorialInTheHomeDirectory)
{
	Terminal terminal;
	terminal.Seed();

	terminal.Tutorial("cyber-tutorial", {{"start", "print(\"welcome\")"}});

	terminal.Submit("cd cyber-tutorial");
	terminal.Submit("run start");
	terminal.Update();

	EXPECT_NE(terminal.GetPrompt().find("/home/user/naomi/cyber-tutorial"), String::npos);
	EXPECT_TRUE(Printed(terminal, "welcome"));
}

TEST(ShellCyber, KeepsAPlayersChangesToTheTutorial)
{
	Terminal terminal;
	terminal.Seed();

	terminal.Tutorial("cyber-tutorial", {{"start", "print(\"welcome\")"}});
	terminal.Submit("cd cyber-tutorial");
	terminal.WriteFile("start", "print(\"mine\")");

	terminal.Tutorial("cyber-tutorial", {{"start", "print(\"welcome\")"}, {"crack", "print(\"crack\")"}});

	String source;
	EXPECT_TRUE(terminal.ReadFile("start", source));
	EXPECT_EQ(source, "print(\"mine\")");
	EXPECT_TRUE(terminal.ReadFile("crack", source));
}

TEST(ShellCyber, RefusesATutorialOutsideTheHomeDirectory)
{
	Terminal terminal;
	terminal.Seed();

	terminal.Tutorial("../escape", {{"start", "print(\"welcome\")"}});
	terminal.Submit("ls");

	EXPECT_FALSE(Printed(terminal, "escape/"));
}
} // namespace TGX::Shell
