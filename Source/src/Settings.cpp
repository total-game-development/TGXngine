#include "Settings.h"
#include <SFML/Window.hpp>
#include <nlohmann/json.hpp>
#include <fstream>
#include "Debug.h"
#include "Globals.h"
#include "WorldState.h"

using namespace nlohmann;

namespace TGX
{
Settings::Settings()
{
	WorldState &world = WorldState::GetInstance();

	String settings_path = "Resources/settings.json";

	if (!std::filesystem::exists(settings_path))
	{
		Log::Error("Settings file missing: " + settings_path);
		return;
	}

	json json_settings;
	std::ifstream settings_stream(settings_path);

	if (!(settings_stream >> json_settings))
	{
		Log::Error("Failed to parse settings JSON: " + settings_path);
		return;
	}

	if (!json_settings.contains("settings") || !json_settings["settings"].contains("production"))
	{
		Log::Error("Key 'settings/production' missing in settings.json");
		return;
	}

	const bool production = json_settings["settings"]["production"];
	world.SetProduction(production);

	if (world.IsProduction())
	{
		UseProduction();
	}

	const bool debugOnScreen = json_settings["settings"]["debugOnScreen"];
	world.SetDebugOnScreen(debugOnScreen && !world.IsProduction());

	const bool fogOfWar = json_settings["settings"].value("fogOfWar", true);
	world.SetFogOfWarEnabled(fogOfWar);
}

void Settings::UseProduction()
{
	WorldState &world = WorldState::GetInstance();

	world.SetProduction(true);
	world.SetDebugOnScreen(false);

	Debug::suppressed = true;

	const std::string extendedPath = "/interface/1920_1080/";
	world.SetExtendedPath(extendedPath);

	const int canvasWidthOffset = 1920 - Globals::canvasWidth;	 // 1040
	const int canvasHeightOffset = 1080 - Globals::canvasHeight; // 720

	world.SetCanvasOffsetSize(canvasWidthOffset, canvasHeightOffset);

	Log::Info("Production: fullscreen, and nothing debug drawn over it");
}

Settings &Settings::GetInstance()
{
	static Settings settings;
	return settings;
}
} // namespace TGX
