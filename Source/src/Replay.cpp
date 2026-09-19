#include "Replay.h"
#include <fstream>
#include <map>
#include "MultiplayerSetup.h"
#include "Renderer.h"
#include "Scene/Game.h"
#include "WorldState.h"

namespace TGX
{
namespace
{
// The cadence a networked client reports its checksums on. A recording only
// holds checksums for these ticks.
constexpr Net::Tick SANITY_INTERVAL = 60;

struct Expected
{
	String clientId;
	std::uint64_t world = 0;
	std::uint64_t commands = 0;
};
} // namespace

int RunReplay(const String &path)
{
	std::ifstream stream(path);

	if (!stream.is_open())
	{
		Log::Error("Cannot open the recording " + path);
		return 2;
	}

	const nlohmann::json record = nlohmann::json::parse(stream, nullptr, false);

	if (!record.is_object() || !record.contains("level") || !record["level"].is_object())
	{
		Log::Error(path + " is not a match recording");
		return 2;
	}

	// Built as an observer's client builds a match: the level and seed the
	// server dealt, from the first tick, with no side of its own.
	MultiplayerSetup::active = true;
	MultiplayerSetup::observer = true;
	MultiplayerSetup::team = "observer";
	MultiplayerSetup::seed = record.value("seed", std::uint32_t{0});
	MultiplayerSetup::startTick = 0;
	MultiplayerSetup::level = record["level"];

	// Nothing moves a pointer here, so park it off the map where no unit can
	// be under it.
	WorldState &world = WorldState::GetInstance();
	world.SetGameX(-1.0e6f);
	world.SetGameY(-1.0e6f);

	Renderer &renderer = Renderer::GetInstance();
	renderer.LoadScene(SceneType::Game);

	const Ref<Game> game = renderer.GetGame();

	std::map<Net::Tick, Vector<Net::Command>> commands;

	for (const auto &entry : record.value("commands", nlohmann::json::array()))
	{
		commands[entry.value("tick", Net::Tick{0})].push_back({
			entry.value("uids", Vector<int>{}),
			entry.value("orders", nlohmann::json::object())});
	}

	std::map<Net::Tick, Vector<Expected>> checksums;

	for (const auto &entry : record.value("checksums", nlohmann::json::array()))
	{
		checksums[entry.value("tick", Net::Tick{0})].push_back({
			entry.value("clientId", String{}),
			entry.value("world", std::uint64_t{0}),
			entry.value("commands", std::uint64_t{0})});
	}

	const Net::Tick endTick = record.value("endTick", Net::Tick{0});

	Log::Info("Replaying " + std::to_string(endTick) + " tick(s), " + std::to_string(record.value("commands", nlohmann::json::array()).size()) + " command(s), on " + MultiplayerSetup::level.value("name", String{"?"}) + " with " + std::to_string(world.items.size()) + " item(s)");

	std::size_t checked = 0;
	std::size_t mismatched = 0;

	for (Net::Tick tick = 0; tick < endTick; ++tick)
	{
		const auto due = commands.find(tick);

		game->RunTick(due != commands.end() ? due->second : Vector<Net::Command>{});

		if (tick % SANITY_INTERVAL != 0)
		{
			continue;
		}

		const auto expected = checksums.find(tick);

		if (expected == checksums.end())
		{
			continue;
		}

		const std::uint64_t worldValue = game->WorldDigest();
		const std::uint64_t commandValue = game->CommandDigest();

		for (const Expected &client : expected->second)
		{
			checked++;

			if (client.world == worldValue && client.commands == commandValue)
			{
				continue;
			}

			mismatched++;

			if (mismatched <= 5)
			{
				Log::Error("Tick " + std::to_string(tick) + ": " + client.clientId + " reported world " + std::to_string(client.world) + " and commands " + std::to_string(client.commands) + "; the replay has " + std::to_string(worldValue) + " and " + std::to_string(commandValue));
			}
		}
	}

	if (checked == 0)
	{
		Log::Error("The recording holds no checksums to hold the replay against");
		return 1;
	}

	if (mismatched != 0)
	{
		Log::Error("Replay diverged: " + std::to_string(mismatched) + " of " + std::to_string(checked) + " checksum(s) disagree");
		return 1;
	}

	Log::Success("Replay matches: " + std::to_string(checked) + " checksum(s) over " + std::to_string(endTick) + " tick(s)");
	return 0;
}
} // namespace TGX
