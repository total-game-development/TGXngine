#include "SkirmishLaunch.h"

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include "Logs.h"
#include "SkirmishSetup.h"

namespace TGX
{
namespace
{
using json = nlohmann::json;

Vector<std::size_t> SkirmishLevels(const json &maps)
{
	Vector<std::size_t> levels;

	if (!maps.contains("singleplayer") || !maps["singleplayer"].is_array())
	{
		return levels;
	}

	for (std::size_t index = 0; index < maps["singleplayer"].size(); index++)
	{
		if (maps["singleplayer"][index].value("type", String{}) == "skirmish")
		{
			levels.push_back(index);
		}
	}

	return levels;
}

String Names(const json &maps, const Vector<std::size_t> &levels)
{
	String named;

	for (std::size_t at = 0; at < levels.size(); at++)
	{
		named += (named.empty() ? "" : ", ") + std::to_string(at + 1) + ". " +
				 maps["singleplayer"][levels[at]].value("name", String{});
	}

	return named;
}
} // namespace

bool PrepareSkirmish(const String &map, const String &team)
{
	const String path = "Resources/maps.json";

	if (!std::filesystem::exists(path))
	{
		Log::Error("Skirmish: maps file missing: " + path);
		return false;
	}

	std::ifstream stream(path);
	json maps;

	if (!(stream >> maps))
	{
		Log::Error("Skirmish: failed to parse " + path);
		return false;
	}

	const Vector<std::size_t> levels = SkirmishLevels(maps);

	if (levels.empty())
	{
		Log::Error("Skirmish: no map in maps.json carries \"type\":\"skirmish\"");
		return false;
	}

	std::size_t chosen = levels.front();
	bool found = map.empty();

	if (!found)
	{
		for (std::size_t at = 0; at < levels.size(); at++)
		{
			if (maps["singleplayer"][levels[at]].value("name", String{}) == map)
			{
				chosen = levels[at];
				found = true;
				break;
			}
		}
	}

	if (!found)
	{
		const int place = std::atoi(map.c_str());

		if (place >= 1 && static_cast<std::size_t>(place) <= levels.size())
		{
			chosen = levels[static_cast<std::size_t>(place) - 1];
			found = true;
		}
	}

	if (!found)
	{
		Log::Error("Skirmish: no map called " + map + ". There is " + Names(maps, levels));
		return false;
	}

	const json &level = maps["singleplayer"][chosen];

	SkirmishSetup::Clear();

	SkirmishSetup::active = true;
	SkirmishSetup::level = static_cast<int>(chosen);

	bool seated = team.empty();

	if (level.contains("teams") && level["teams"].is_array())
	{
		for (const auto &side : level["teams"])
		{
			const String name = side.value("name", String{});
			String role = side.value("type", String{"none"});

			if (!team.empty())
			{
				role = (name == team) ? String{"player"} : String{"ai"};

				seated = seated || (name == team);
			}

			SkirmishSetup::slots.push_back({name, role});
		}
	}

	if (!seated)
	{
		String sides;

		for (const SkirmishSetup::Slot &slot : SkirmishSetup::slots)
		{
			sides += (sides.empty() ? "" : ", ") + slot.team;
		}

		Log::Error("Skirmish: " + level.value("name", String{}) + " has no side called " + team + ". It has " + sides);

		SkirmishSetup::Clear();

		return false;
	}

	SkirmishSetup::spectator = SkirmishSetup::PlayerTeam().empty();

	Log::Success("Skirmish starting map " + level.value("name", String{}) +
				 (SkirmishSetup::spectator ? " (watched)" : " as " + SkirmishSetup::PlayerTeam()));

	return true;
}
} // namespace TGX
