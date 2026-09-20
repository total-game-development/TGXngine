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
#include "SkirmishLaunch.h"
#include "WorldState.h"

namespace TGX
{
void Init(const String &scene, bool production)
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

	// Before the renderer, which builds the window the mode decides the size of.
	if (production)
	{
		Settings::UseProduction();
	}

	[[maybe_unused]] Renderer &renderer = Renderer::GetInstance();

	renderer.LoadScene(scene);

	renderer.Start();
}
} // namespace TGX

namespace
{
void Usage()
{
	std::cout << "TGXngine\n"
			  << "  --skirmish [<map>]      start a skirmish instead of the menu\n"
			  << "  --map <name|number>     which skirmish map, by name or by place\n"
			  << "  --team <name>           the side to command; left out, the map decides\n"
			  << "  --replay <file>         replay a recorded match\n"
			  << "  --host <url>            host a networked match headlessly\n"
			  << "    --room <number>       the room to host\n"
			  << "    --token <token>       the token the server started it with\n"
			  << "    --audit <ticks>       how often to audit the world's rules, 0 for never\n";
}
} // namespace

bool Asked(int argc, char **argv, const TGX::String &flag)
{
	for (int index = 1; index < argc; index++)
	{
		if (flag == argv[index])
		{
			return true;
		}
	}

	return false;
}

int main(int argc, char **argv)
{
	if (Asked(argc, argv, "--help") || Asked(argc, argv, "-h"))
	{
		Usage();
		return 0;
	}

	const bool production = Asked(argc, argv, "--production");

	if (argc >= 3 && TGX::String(argv[1]) == "--replay")
	{
		return TGX::RunReplay(argv[2]);
	}

	if (argc >= 2 && TGX::String(argv[1]) == "--host")
	{
		TGX::String url = "ws://127.0.0.1:9001";
		TGX::String token;
		int room = 0;
		int audit = 60;

		for (int index = 1; index + 1 < argc; index++)
		{
			const TGX::String flag = argv[index];

			if (flag == "--host") { url = argv[++index]; }
			else if (flag == "--room") { room = std::atoi(argv[++index]); }
			else if (flag == "--token") { token = argv[++index]; }
			else if (flag == "--audit") { audit = std::atoi(argv[++index]); }
		}

		return TGX::RunHost(url, room, token, audit);
	}

	if (argc >= 2 && TGX::String(argv[1]) == "--skirmish")
	{
		TGX::String map;
		TGX::String team;

		for (int index = 1; index + 1 < argc; index++)
		{
			const TGX::String flag = argv[index];

			if (flag == "--skirmish" || flag == "--map") { map = argv[++index]; }
			else if (flag == "--team") { team = argv[++index]; }
		}

		if (!TGX::PrepareSkirmish(map, team))
		{
			return 1;
		}

		TGX::Init(TGX::String("game"), production);

		return 0;
	}

	TGX::Init(TGX::String("intro"), production);

	return 0;
}
