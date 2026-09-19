#include <SFML/Graphics.hpp>
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <iostream>
#include "Controller.h"
#include "ImageLoader.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "Physics.h"
#include "Host.h"
#include "Renderer.h"
#include "Replay.h"
#include "Settings.h"
#include "WorldState.h"

namespace TGX
{
void Init()
{
	std::cout << std::boolalpha;

	Log::Info("Info");
	Log::Print("Print");
	Log::Debug("Debug");
	Log::Success("Success");
	Log::Warning("Warning");
	Log::Error("Error");
	Log::Clean("Clean");
	Log::Crash("Crash");

	// call all the singleton constructors in case they have important side effects
	[[maybe_unused]] ImageLoader &imageLoader = ImageLoader::GetInstance();
	[[maybe_unused]] Mouse &mouse = Mouse::GetInstance();
	[[maybe_unused]] Keyboard &keyboard = Keyboard::GetInstance();
	[[maybe_unused]] Controller &controller = Controller::GetInstance();
	[[maybe_unused]] Physics &physics = Physics::GetInstance();
	[[maybe_unused]] WorldState &worldState = WorldState::GetInstance();
	[[maybe_unused]] Settings &settings = Settings::GetInstance();
	[[maybe_unused]] Renderer &renderer = Renderer::GetInstance();

	renderer.LoadScene(String("intro"));

	renderer.Start();
}
} // namespace TGX

int main(int argc, char **argv)
{
	if (argc >= 3 && TGX::String(argv[1]) == "--replay")
	{
		return TGX::RunReplay(argv[2]);
	}

	if (argc >= 2 && TGX::String(argv[1]) == "--host")
	{
		TGX::String url = "ws://127.0.0.1:9001";
		TGX::String token;
		int room = 0;

		for (int index = 1; index + 1 < argc; index++)
		{
			const TGX::String flag = argv[index];

			if (flag == "--host") { url = argv[++index]; }
			else if (flag == "--room") { room = std::atoi(argv[++index]); }
			else if (flag == "--token") { token = argv[++index]; }
		}

		return TGX::RunHost(url, room, token);
	}

	TGX::Init();

	return 0;
}
