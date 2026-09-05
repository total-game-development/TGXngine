#include <SFML/Graphics.hpp>
#include "Enums.h"
#include "Sidebar.h"
#include "StringUtils.hpp"
#include "Utils.hpp"
#include "Window.h"
#include "WorldState.h"
#include "module_interface.h"

namespace TGX
{

Sidebar &GetSidebar()
{
	static Sidebar sidebar;
	return sidebar;
}

extern "C"
{
	MODULE_API void OutputTest()
	{
	}

	MODULE_API void Init()
	{
		GetSidebar();

		Log::Success("Interface Init");
	}

	MODULE_API void Awake(const std::string &name)
	{
		if (name == "sidebar")
		{
			GetSidebar().Load(name);
		}

		Log::Info("Interface Name: " + name);
	}

	MODULE_API void Create()
	{
	}

	MODULE_API void Draw()
	{
		GetSidebar().Draw();

		Window &window = Window::GetInstance();
		WorldState &worldState = WorldState::GetInstance();

		String cashValue = std::to_string(worldState.GetCash());
		const String &viewing = worldState.GetTeam();
		String powerValue = std::to_string(worldState.GetPowerUsage(viewing)) + " / " + std::to_string(worldState.GetPowerTotal(viewing));

		window.DrawText("Power: " + powerValue, {static_cast<float>(worldState.GetCanvasWidth() + worldState.GetCanvasOffsetWidth() - 300), 33.f}, 14, sf::Color::Yellow);
		window.DrawText("Cash: $" + cashValue, {static_cast<float>(worldState.GetCanvasWidth() + worldState.GetCanvasOffsetWidth() - 300), 51.5f}, 14, sf::Color::Yellow);
	}

	MODULE_API void Update()
	{
		GetSidebar().Update();
	}

	MODULE_API void Click()
	{
		WorldState &world = WorldState::GetInstance();
		if (world.IsPlacement())
		{
			if (world.IsBuilt())
			{
				Log::Success("Successful placement and built");
				world.SetPlacement(false);

				String command = StringConcat("command:", "build");
				command += ",";
				String name = StringConcat("name:", StringSplit(world.pendingQueue, ",")[0]);
				command += name + ",";
				String type = StringConcat("type:", StringSplit(world.pendingQueue, ",")[1]);
				command += type + ",";
				String team = StringConcat("team:", world.GetTeam());
				command += team + ",";
				String x = StringConcat("x:", RoundGridDown(world.GetGameX(), 0, 20));
				command += x + ",";
				String y = StringConcat("y:", RoundGridDown(world.GetGameY(), -80, 20));
				command += y;

				Log::Info("world.GetX() = " + std::to_string(world.GetGameX()));
				Log::Info("world.GetY() = " + std::to_string(world.GetGameY()));
				Log::Info("RoundGridDown(world.GetX()) = " + std::to_string(RoundGridDown(world.GetGameX(), 0, 20)));
				Log::Info("RoundGridDown(world.GetY()) = " + std::to_string(RoundGridDown(world.GetGameY(), -80, 20)));

				Log::Info("NI Command: " + command);

				world.gameEvents.emplace_back(UIAction::AddGameItem, command);

				GetSidebar().Restore();

				world.pendingQueue.clear();
			}
			else
			{
				Log::Warning("Successful placement, but invalid built");
			}
		}

		GetSidebar().Click();
	}

	MODULE_API void Clear()
	{
		Log::Success("Clear Interface");
	}

	MODULE_API void Delete()
	{
		Log::Clean("Delete Interface");
	}

	MODULE_API void Destroy(const std::string &name)
	{
		Log::Clean("Destroy interface " + name);
	}
}
} // namespace TGX
