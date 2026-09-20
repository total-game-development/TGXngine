#pragma once

namespace TGX
{
class Settings
{
private:
	Settings();
	~Settings() = default;

public:
	static Settings &GetInstance();

	// Fullscreen, and the interface laid out for it. settings.json asks for it
	// with "production": true, and --production asks for it from the command
	// line, which has to land before the window is built.
	static void UseProduction();

	Settings(const Settings &) = delete;
	Settings &operator=(const Settings &) = delete;

	Settings(Settings &&) = delete;
	Settings &operator=(Settings &&) = delete;
};
} // namespace TGX
