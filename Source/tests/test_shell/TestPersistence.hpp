#pragma once

#include <cstdio>
#include <gtest/gtest.h>
#include "Terminal.h"

namespace TGX::Shell
{
inline String TempSavePath()
{
	return "shell_persistence_test.json";
}

TEST(ShellPersistence, RestoresFilesWrittenInAnEarlierSession)
{
	const String path = TempSavePath();
	std::remove(path.c_str());

	{
		Terminal terminal;
		terminal.Load(path);
		terminal.Submit("mk hello");
		terminal.Submit("edit hello");
		terminal.WriteFile("hello", "print(\"Hello\")");
		terminal.Save(path);
	}

	{
		Terminal terminal;
		terminal.Load(path);

		String source;
		EXPECT_TRUE(terminal.ReadFile("hello", source));
		EXPECT_EQ(source, "print(\"Hello\")");
	}

	std::remove(path.c_str());
}

TEST(ShellPersistence, RestoresTheWorkingDirectory)
{
	const String path = TempSavePath();
	std::remove(path.c_str());

	{
		Terminal terminal;
		terminal.Load(path);
		terminal.Submit("mkdir bin");
		terminal.Submit("cd bin");
		terminal.Save(path);
	}

	{
		Terminal terminal;
		terminal.Load(path);
		EXPECT_NE(terminal.GetPrompt().find("/home/user/naomi/bin/"), String::npos);
	}

	std::remove(path.c_str());
}

TEST(ShellPersistence, SeedsAFreshTreeWhenNoSaveExists)
{
	const String path = "shell_persistence_missing.json";
	std::remove(path.c_str());

	Terminal terminal;
	terminal.Load(path);

	EXPECT_NE(terminal.GetPrompt().find("/home/user/naomi/"), String::npos);
}


TEST(ShellPersistence, SavesAsSoonAsAFileIsCreated)
{
	const String path = "shell_persistence_autosave.json";
	std::remove(path.c_str());

	{
		Terminal terminal;
		terminal.Load(path);
		terminal.Submit("mk notes");
	}

	{
		Terminal terminal;
		terminal.Load(path);
		String source;
		EXPECT_TRUE(terminal.ReadFile("notes", source));
	}

	std::remove(path.c_str());
}

TEST(ShellPersistence, SavesDirectoriesAsSoonAsTheyAreMade)
{
	const String path = "shell_persistence_mkdir.json";
	std::remove(path.c_str());

	{
		Terminal terminal;
		terminal.Load(path);
		terminal.Submit("mkdir bin");
	}

	{
		Terminal terminal;
		terminal.Load(path);
		terminal.Submit("cd bin");
		EXPECT_NE(terminal.GetPrompt().find("bin"), String::npos);
	}

	std::remove(path.c_str());
}

TEST(ShellPersistence, KeepsEditsWhenQuittingFromTheEditor)
{
	const String path = "shell_persistence_editor.json";
	std::remove(path.c_str());

	{
		Terminal terminal;
		terminal.Load(path);
		terminal.Submit("mk draft");
		terminal.Submit("edit draft");

		ASSERT_EQ(terminal.GetMode(), TerminalMode::Editing);

		terminal.Character('h');
		terminal.Character('i');

		terminal.CommitEditor();
	}

	{
		Terminal terminal;
		terminal.Load(path);

		String source;
		ASSERT_TRUE(terminal.ReadFile("draft", source));
		EXPECT_EQ(source, "hi");
	}

	std::remove(path.c_str());
}

TEST(ShellPersistence, ProgramWritesReachDisk)
{
	const String path = "shell_persistence_write.json";
	std::remove(path.c_str());

	{
		Terminal terminal;
		terminal.Load(path);
		terminal.WriteFile("generated", "print(1)");
	}

	{
		Terminal terminal;
		terminal.Load(path);

		String source;
		ASSERT_TRUE(terminal.ReadFile("generated", source));
		EXPECT_EQ(source, "print(1)");
	}

	std::remove(path.c_str());
}
} // namespace TGX::Shell
