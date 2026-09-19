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
			// Asked again here rather than read back from world.IsBuilt(), which
			// the last frame worked out while it drew. The pointer moves between
			// the two, so that answer is about where the cursor used to be, and
			// a quick move onto something impassable would be built on anyway.
			if (GetSidebar().PlacementFits())
			{
				Log::Success("Successful placement and built");
				world.SetPlacement(false);

				String command = StringConcat("team:", world.GetTeam());
				command += ",";
				String name = StringConcat("key:", StringSplit(world.pendingQueue, ",")[0]);
				command += name + ",";
				String x = StringConcat("x:", RoundGridDown(world.GetGameX(), 0, 20));
				command += x + ",";
				String y = StringConcat("y:", RoundGridDown(world.GetGameY(), -80, 20));
				command += y;

				Log::Info("world.GetX() = " + std::to_string(world.GetGameX()));
				Log::Info("world.GetY() = " + std::to_string(world.GetGameY()));
				Log::Info("RoundGridDown(world.GetX()) = " + std::to_string(RoundGridDown(world.GetGameX(), 0, 20)));
				Log::Info("RoundGridDown(world.GetY()) = " + std::to_string(RoundGridDown(world.GetGameY(), -80, 20)));

				Log::Info("NI Command: " + command);

				world.gameEvents.emplace_back(UIAction::PlayerPlace, command);

				GetSidebar().Restore();

				world.pendingQueue.clear();
			}
			else
			{
				// It is paid for and still waiting to be put down, so the click
				// is a try that missed rather than the end of the placement.
				// Keeping the selection keeps the producing building on the
				// sidebar, which is what restores the ghost for the next try.
				Log::Warning("Nothing can be built there");

				world.SetSkipSelectionRemoval(true);
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
