#pragma once

#include <gtest/gtest.h>
#include "FileSystem.h"

namespace TGX::Shell
{
TEST(ShellFileSystem, StartsAtRoot)
{
	FileSystem fileSystem;

	EXPECT_EQ(fileSystem.GetCurrentDirectory(), "/");
	EXPECT_TRUE(fileSystem.List().empty());
}

TEST(ShellFileSystem, MakesAndEntersDirectories)
{
	FileSystem fileSystem;
	String message;

	EXPECT_TRUE(fileSystem.MakeDirectory("home", message));
	EXPECT_TRUE(fileSystem.ChangeDirectory("home", message));
	EXPECT_EQ(fileSystem.GetCurrentDirectory(), "/home/");
}

TEST(ShellFileSystem, RejectsDuplicateDirectories)
{
	FileSystem fileSystem;
	String message;

	EXPECT_TRUE(fileSystem.MakeDirectory("home", message));
	EXPECT_FALSE(fileSystem.MakeDirectory("home", message));
	EXPECT_EQ(message, "Directory already exists");
}

TEST(ShellFileSystem, WalksBackToParent)
{
	FileSystem fileSystem;
	String message;

	fileSystem.MakeDirectory("home", message);
	fileSystem.ChangeDirectory("home", message);
	fileSystem.MakeDirectory("user", message);
	fileSystem.ChangeDirectory("user", message);

	EXPECT_EQ(fileSystem.GetCurrentDirectory(), "/home/user/");

	EXPECT_TRUE(fileSystem.ChangeDirectory("..", message));
	EXPECT_EQ(fileSystem.GetCurrentDirectory(), "/home/");

	EXPECT_TRUE(fileSystem.ChangeDirectory("/", message));
	EXPECT_EQ(fileSystem.GetCurrentDirectory(), "/");
}

TEST(ShellFileSystem, RefusesToLeaveRoot)
{
	FileSystem fileSystem;
	String message;

	EXPECT_FALSE(fileSystem.ChangeDirectory("..", message));
	EXPECT_EQ(fileSystem.GetCurrentDirectory(), "/");
}

TEST(ShellFileSystem, RefusesToEnterAFile)
{
	FileSystem fileSystem;
	String message;

	fileSystem.MakeFile("program", message);

	EXPECT_FALSE(fileSystem.ChangeDirectory("program", message));
	EXPECT_EQ(message, "program is a file");
}

TEST(ShellFileSystem, WritesAndReadsFiles)
{
	FileSystem fileSystem;
	String source;

	fileSystem.Write("program", "print(1)");

	ASSERT_TRUE(fileSystem.Read("program", source));
	EXPECT_EQ(source, "print(1)");
}

TEST(ShellFileSystem, DoesNotReadDirectoriesAsFiles)
{
	FileSystem fileSystem;
	String message;
	String source;

	fileSystem.MakeDirectory("home", message);

	EXPECT_FALSE(fileSystem.Read("home", source));
	EXPECT_TRUE(fileSystem.IsDirectory("home"));
}

TEST(ShellFileSystem, RenamesAndRemovesEntries)
{
	FileSystem fileSystem;
	String message;

	fileSystem.Write("old", "body");

	EXPECT_TRUE(fileSystem.Rename("old", "new", message));
	EXPECT_TRUE(fileSystem.Exists("new"));
	EXPECT_FALSE(fileSystem.Exists("old"));

	EXPECT_TRUE(fileSystem.Remove("new", message));
	EXPECT_FALSE(fileSystem.Exists("new"));
}

TEST(ShellFileSystem, ListsDirectoriesWithTrailingSlash)
{
	FileSystem fileSystem;
	String message;

	fileSystem.MakeDirectory("home", message);
	fileSystem.Write("program", "print(1)");

	const Vector<String> entries = fileSystem.List();

	ASSERT_EQ(entries.size(), 2u);
	EXPECT_EQ(entries[0], "home/");
	EXPECT_EQ(entries[1], "program");
}

TEST(ShellFileSystem, RoundTripsThroughJson)
{
	FileSystem fileSystem;
	String message;

	fileSystem.MakeDirectory("home", message);
	fileSystem.ChangeDirectory("home", message);
	fileSystem.Write("program", "print(1)");

	const nlohmann::json data = fileSystem.Serialise();

	FileSystem restored;
	restored.Deserialise(data);

	EXPECT_EQ(restored.GetCurrentDirectory(), "/home/");

	String source;
	ASSERT_TRUE(restored.Read("program", source));
	EXPECT_EQ(source, "print(1)");
}
} // namespace TGX::Shell
