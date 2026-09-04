#include "AI.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include "Core.h"
#include "Logs.h"
#include "module_interface.h"

namespace TGX
{
// Allocate the actual storage space memory exactly ONCE inside this single implementation object
Vector<AIState *> activeOpponentStates;

static const Map<String, StateFactory> StateRegistry = {
	{"plex", []() -> AIState * { return new PlexAIState(); }},
	{"builder", []() -> AIState * { return new BuilderAIState(); }}};

extern "C"
{
	MODULE_API void Init() {}

	MODULE_API void Awake(const String &name)
	{
		Log::Success("AI Core Awake Invoked.");

		WorldState &world = WorldState::GetInstance();
		int currentLevel = world.GetCurrentLevel();

		String mapsPath = "Resources/maps.json";
		if (!std::filesystem::exists(mapsPath))
		{
			Log::Error("AI initialization cancelled. Maps configuration missing: " + mapsPath);
			return;
		}

		nlohmann::json mapsJson;
		std::ifstream mapsStream(mapsPath);
		if (!(mapsStream >> mapsJson))
		{
			Log::Error("AI module failed to parse maps JSON file layout.");
			return;
		}

		if (!mapsJson.contains("singleplayer") || !mapsJson["singleplayer"].is_array() ||
			static_cast<size_t>(currentLevel) >= mapsJson["singleplayer"].size())
		{
			Log::Error("AI Awake: Current level index out of bounds inside configuration matrices.");
			return;
		}

		const auto &levelData = mapsJson["singleplayer"][currentLevel];
		if (!levelData.contains("ai") || !levelData["ai"].is_array() || levelData["ai"].empty())
		{
			Log::Print("No AI operational profiles assigned for level " + std::to_string(currentLevel));
			return;
		}

		if (!levelData.contains("teams") || !levelData["teams"].is_array())
		{
			Log::Error("AI Awake: level carries AI profiles but no team list to attach them to.");
			return;
		}

		const auto &profiles = levelData["ai"];
		std::size_t profileIndex = 0;

		for (const auto &teamEntry : levelData["teams"])
		{
			if (teamEntry.value("type", String{}) != "ai")
			{
				continue;
			}

			const auto &aiOpponent = profiles[std::min(profileIndex, profiles.size() - 1)];
			profileIndex++;

			String aiTypeName = aiOpponent.value("name", name.empty() ? String{"builder"} : name);

			auto registryIterator = StateRegistry.find(aiTypeName);
			if (registryIterator == StateRegistry.end())
			{
				Log::Error("AI Engine Error: Unrecognized state type requested from config: " + aiTypeName);
				continue;
			}

			// Allocate a dedicated instance for this distinct runtime commander profile
			AIState *newOpponentState = registryIterator->second();

			String opponentTeam = teamEntry.value("name", String{});
			newOpponentState->SetTeam(opponentTeam);

			if (levelData.contains("economy") && levelData["economy"].contains(opponentTeam))
			{
				newOpponentState->SetCash(levelData["economy"][opponentTeam].value("cash", 0));
			}

			// Setup state tree data polymorphically
			newOpponentState->InitialiseMapTechTree(aiOpponent);
			newOpponentState->Awake();

			// Track this instance in our central vector state table
			activeOpponentStates.push_back(newOpponentState);
			Log::Success("Dynamically instantiated AI state object target type: " + aiTypeName +
						 " commanding " + opponentTeam + " with $" + std::to_string(newOpponentState->GetCash()));
		}

		Log::Success("AI Awake routine completed successfully.");
	}

	MODULE_API void Start()
	{
		// Broadcast game loop processing updates down to all initialized AI drivers
		for (auto *state : activeOpponentStates)
		{
			if (state)
			{
				state->Start();
			}
		}
		Log::Success("AI Module Start Process Completed.");
	}

	MODULE_API void Update()
	{
		for (auto *state : activeOpponentStates)
		{
			if (state)
			{
				state->Update();
			}
		}
	}

	MODULE_API void Clear()
	{
		// Cleanly wipe heap spaces allocations for every spawned entity track item
		for (auto *state : activeOpponentStates)
		{
			delete state;
		}
		activeOpponentStates.clear();
		Log::Success("AI states collection flushed clean.");
	}

	MODULE_API void Destroy() {}
}
} // namespace TGX
