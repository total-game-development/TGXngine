#pragma once

#include <gtest/gtest.h>
#include "Terminal.h"
#include "TestRemote.hpp"
#include "Value.h"

namespace TGX::Shell
{
struct RadarFixture : RemoteFixture
{
	nlohmann::json positions = {
		{"width", 20},
		{"height", 20},
		{"cut", false},
		{"cells", nlohmann::json::array({{3, 4}, {10, 12}, {10, 12}})}};

	Vector<std::tuple<int, int, int>> shown;

	void SetUp() override
	{
		RemoteFixture::SetUp();

		sapphire.SetRadar({2, 1, std::chrono::milliseconds(0), std::chrono::milliseconds(120000)});
		sapphire.SetRadarHandlers([this]() { return positions; }, nullptr);

		ruby.SetRadarHandlers(nullptr, [this](int x, int y, int cell) { shown.emplace_back(x, y, cell); });

		sapphire.Update();
	}

	String Radar(Terminal &terminal)
	{
		String source;

		Run(terminal, "cd /");
		Run(terminal, "cd sys");
		terminal.ReadFile("radar", source);
		Run(terminal, "cd /");

		return source;
	}

	void Steal()
	{
		Run(ruby, "connect sapphire 2222");
		Run(ruby, "cd sys");
		Run(ruby, "get radar");
		Run(ruby, "disconnect");
	}

	static String Crack(const String &radar, int digits)
	{
		const std::size_t at = radar.find("check ");
		const String check = radar.substr(at + 6, 8);

		int limit = 1;

		for (int index = 0; index < digits; ++index)
		{
			limit *= 10;
		}

		for (int candidate = 0; candidate < limit; ++candidate)
		{
			String salt = std::to_string(candidate);

			while (static_cast<int>(salt.size()) < digits)
			{
				salt.insert(salt.begin(), '0');
			}

			if (Digest({salt}) == check)
			{
				return salt;
			}
		}

		return String();
	}
};

TEST_F(RadarFixture, PublishesItsPositionsHashed)
{
	const String radar = Radar(sapphire);

	EXPECT_EQ(radar.rfind("map 20 20 1\ncheck ", 0), 0u);

	const String salt = Crack(radar, 2);
	ASSERT_EQ(salt.size(), 2u);

	EXPECT_NE(radar.find(Digest({salt, "3", "4"})), String::npos);
	EXPECT_NE(radar.find(Digest({salt, "10", "12"})), String::npos);
	EXPECT_EQ(std::ranges::count(radar, '\n'), 4);
}

TEST_F(RadarFixture, GetCopiesAFileAcross)
{
	Steal();

	EXPECT_TRUE(Printed(ruby, "Copied radar from sapphire"));
	EXPECT_TRUE(Printed(sapphire, "ruby copied radar from this computer"));

	String copy;
	ASSERT_TRUE(ruby.ReadFile("radar", copy));
	EXPECT_EQ(copy, Radar(sapphire));
}

TEST_F(RadarFixture, GetNeedsASession)
{
	Run(ruby, "get radar");

	EXPECT_TRUE(Printed(ruby, "Connect to somebody first"));
}

TEST_F(RadarFixture, ACrackedSaltRevealsWhereThingsStand)
{
	Steal();

	String copy;
	ASSERT_TRUE(ruby.ReadFile("radar", copy));

	const String salt = Crack(copy, 2);

	EXPECT_TRUE(ruby.Reveal(salt, 3, 4));
	EXPECT_TRUE(ruby.Reveal(salt, 10, 12));
	EXPECT_FALSE(ruby.Reveal(salt, 4, 4));

	EXPECT_TRUE(shown.empty());

	ruby.Update();

	ASSERT_EQ(shown.size(), 2u);
	EXPECT_EQ(shown[0], std::make_tuple(3, 4, 1));
	EXPECT_EQ(shown[1], std::make_tuple(10, 12, 1));
}

TEST_F(RadarFixture, TheWrongSaltRevealsNothing)
{
	Steal();

	String copy;
	ASSERT_TRUE(ruby.ReadFile("radar", copy));

	const String salt = Crack(copy, 2);
	const String wrong = salt == "00" ? "01" : "00";

	EXPECT_FALSE(ruby.Reveal(wrong, 3, 4));
}

TEST_F(RadarFixture, AProgramCracksItFromTheConsole)
{
	Steal();

	ruby.WriteFile("crack",
				   "let text = read(\"radar\")\n"
				   "let lines = []\n"
				   "split(text, \"\n\", lines)\n"
				   "let head = []\n"
				   "split(lines[0], \" \", head)\n"
				   "let check = []\n"
				   "split(lines[1], \" \", check)\n"
				   "let w = head[1] * 1\n"
				   "let h = head[2] * 1\n"
				   "let cell = head[3] * 1\n"
				   "let digits = args[0] * 1\n"
				   "let limit = 1\n"
				   "let i = 0\n"
				   "while (i < digits) { limit = limit * 10 i = i + 1 }\n"
				   "let salt = \"\"\n"
				   "let n = 0\n"
				   "let s = \"\"\n"
				   "while (n < limit) {\n"
				   "  s = \"\" + n\n"
				   "  while (length(s) < digits) { s = \"0\" + s }\n"
				   "  if (hash(s) == check[1]) { salt = s break }\n"
				   "  n = n + 1\n"
				   "}\n"
				   "let hits = 0\n"
				   "let x = 0\n"
				   "let y = 0\n"
				   "let k = 0\n"
				   "let c = \"\"\n"
				   "while (y < h / cell) {\n"
				   "  x = 0\n"
				   "  while (x < w / cell) {\n"
				   "    c = hash(salt, x, y)\n"
				   "    k = 2\n"
				   "    while (k < length(lines)) {\n"
				   "      if (lines[k] == c) { reveal(salt, x, y) hits = hits + 1 }\n"
				   "      k = k + 1\n"
				   "    }\n"
				   "    x = x + 1\n"
				   "  }\n"
				   "  y = y + 1\n"
				   "}\n"
				   "print(\"revealed \" + hits)\n");

	Run(ruby, "run crack 2");
	ruby.Update();

	EXPECT_TRUE(Printed(ruby, "revealed 2"));
	ASSERT_EQ(shown.size(), 2u);
	EXPECT_EQ(shown[0], std::make_tuple(3, 4, 1));
	EXPECT_EQ(shown[1], std::make_tuple(10, 12, 1));
}

TEST_F(RadarFixture, AForgedRadarProvesNothing)
{
	ruby.WriteFile("radar", "map 20 20 1\ncheck " + Digest({"99"}) + "\n" + Digest({"99", "5", "5"}) + "\n");

	EXPECT_FALSE(ruby.Reveal("99", 5, 5));

	Steal();

	ruby.WriteFile("radar", "map 20 20 1\ncheck " + Digest({"99"}) + "\n" + Digest({"99", "5", "5"}) + "\n");

	EXPECT_FALSE(ruby.Reveal("99", 5, 5));
}

TEST_F(RadarFixture, OnlyTheLiveRadarIsKept)
{
	Run(sapphire, "cd /");
	sapphire.WriteFile("notes", "map 20 20 1\ncheck " + Digest({"99"}) + "\n" + Digest({"99", "5", "5"}) + "\n");

	Run(ruby, "connect sapphire 2222");
	Run(ruby, "get notes");
	Run(ruby, "disconnect");

	EXPECT_TRUE(Printed(ruby, "Copied notes from sapphire"));
	EXPECT_FALSE(ruby.Reveal("99", 5, 5));
}

TEST_F(RadarFixture, RekeyingMakesACrackStale)
{
	Steal();

	String copy;
	ASSERT_TRUE(ruby.ReadFile("radar", copy));

	const String salt = Crack(copy, 2);
	String fresh;

	for (int attempt = 0; attempt < 20; ++attempt)
	{
		Run(sapphire, "rekey");

		if (Crack(Radar(sapphire), 2) != salt)
		{
			fresh = Radar(sapphire);
			break;
		}
	}

	ASSERT_FALSE(fresh.empty());
	EXPECT_TRUE(Printed(sapphire, "Radar rekeyed"));

	Steal();

	EXPECT_FALSE(ruby.Reveal(salt, 3, 4));
	EXPECT_TRUE(ruby.Reveal(Crack(fresh, 2), 3, 4));
}

TEST_F(RadarFixture, ACutGridCannotBeRekeyed)
{
	const String before = Radar(sapphire);

	positions["cut"] = true;

	Run(sapphire, "rekey");
	sapphire.Update();

	EXPECT_TRUE(Printed(sapphire, "The grid is cut, so the radar cannot be rekeyed until it is restored"));
	EXPECT_EQ(Crack(Radar(sapphire), 2), Crack(before, 2));
}

TEST_F(RadarFixture, ACutGridStopsTheSaltRotating)
{
	sapphire.SetRadar({2, 1, std::chrono::milliseconds(0), std::chrono::milliseconds(0)});
	positions["cut"] = true;
	sapphire.Update();

	const String salt = Crack(Radar(sapphire), 2);

	for (int attempt = 0; attempt < 20; ++attempt)
	{
		sapphire.Update();
		EXPECT_EQ(Crack(Radar(sapphire), 2), salt);
	}

	positions["cut"] = false;

	bool rotated = false;

	for (int attempt = 0; attempt < 20 && !rotated; ++attempt)
	{
		sapphire.Update();
		rotated = Crack(Radar(sapphire), 2) != salt;
	}

	EXPECT_TRUE(rotated);
}

TEST_F(RadarFixture, TheRadarIsRekeyedFromItsOwnConsoleOnly)
{
	Run(ruby, "connect sapphire 2222");
	Run(ruby, "rekey");

	EXPECT_TRUE(Printed(ruby, "The radar on sapphire can only be rekeyed from its own console"));
	EXPECT_FALSE(Printed(sapphire, "Radar rekeyed"));
}

TEST_F(RadarFixture, RevealIsRefusedFromARemoteSession)
{
	sapphire.SetRadarHandlers([this]() { return positions; }, [this](int x, int y, int cell) { shown.emplace_back(x, y, cell); });

	Run(ruby, "connect sapphire 2222");
	Run(sapphire, "cd /");
	sapphire.WriteFile("peek", "print(reveal(\"00\", 3, 4))");

	Run(ruby, "run peek");
	sapphire.Update();

	EXPECT_TRUE(Printed(ruby, "reveal is refused from a remote session"));
	EXPECT_TRUE(shown.empty());
}

TEST_F(RadarFixture, CoarseCellsRevealAnArea)
{
	sapphire.SetRadar({2, 4, std::chrono::milliseconds(0), std::chrono::milliseconds(120000)});
	sapphire.Update();

	const String radar = Radar(sapphire);
	EXPECT_EQ(radar.rfind("map 20 20 4\n", 0), 0u);

	Steal();

	const String salt = Crack(radar, 2);

	EXPECT_TRUE(ruby.Reveal(salt, 2, 3));
	EXPECT_FALSE(ruby.Reveal(salt, 10, 12));

	ruby.Update();

	ASSERT_EQ(shown.size(), 1u);
	EXPECT_EQ(shown[0], std::make_tuple(2, 3, 4));
}

TEST_F(RadarFixture, LeavingTheNetworkForgetsWhatWasTaken)
{
	Steal();

	String copy;
	ASSERT_TRUE(ruby.ReadFile("radar", copy));

	const String salt = Crack(copy, 2);

	ruby.ClearNetwork();

	EXPECT_FALSE(ruby.Reveal(salt, 3, 4));
}
} // namespace TGX::Shell
