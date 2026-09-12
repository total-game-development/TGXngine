#include "Keyboard.h"
#include "Debug.h"
#include "Logs.h"
#include "WorldState.h"

namespace TGX
{
Keyboard::Keyboard()
{
	Log::Success("Keyboard Created");
}

Keyboard &Keyboard::GetInstance()
{
	static Keyboard keyboard;
	return keyboard;
}

void Keyboard::KeyPressed(Key code)
{
	Log::Info("Keyboard code : " + std::to_string(code));

	switch (code)
	{
		case Key::G: // Toggle grid
			{
				Debug::showGrid = !Debug::showGrid;
				break;
			}

		case Key::W: // Toggle waypoints
			{
				Debug::showWayPoints = !Debug::showWayPoints;
				break;
			}

		case Key::E: // Toggle the economy readout
			{
				Debug::showEconomy = !Debug::showEconomy;
				break;
			}

		case Key::Q: // Exit game
		case Key::Escape:
			{
				Log::Success("Exit pressed - exiting game.");
				WorldState &world = WorldState::GetInstance();
				world.SetClosed(true);
				break;
			}

		default:
			break;
	}
}
} // namespace TGX
