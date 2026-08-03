#include "Settings.h"
#include <SFML/Window.hpp>
#include <nlohmann/json.hpp>
#include <fstream>
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
		const sf::VideoMode desktopMode = sf::VideoMode::getDesktopMode();

		const int width = static_cast<int>(desktopMode.width);
		const int height = static_cast<int>(desktopMode.height);

		const std::string widthString = std::to_string(width);
		const std::string heightString = std::to_string(height);

		const std::string extendedPath = "/interface/1920_1080/";
		world.SetExtendedPath(extendedPath);

		const int canvasWidthOffset = 1920 - Globals::canvasWidth;	 // 1040
		const int canvasHeightOffset = 1080 - Globals::canvasHeight; // 720

		world.SetCanvasOffsetSize(canvasWidthOffset, canvasHeightOffset);
	}

	const bool debugOnScreen = json_settings["settings"]["debugOnScreen"];
	world.SetDebugOnScreen(debugOnScreen);

	const bool fogOfWar = json_settings["settings"].value("fogOfWar", true);
	world.SetFogOfWarEnabled(fogOfWar);
}

Settings &Settings::GetInstance()
{
	static Settings settings;
	return settings;
}
} // namespace TGX
