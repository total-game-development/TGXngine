#include "Controller.h"
#include <cmath>
#include "Debug.h"
#include "Logs.h"
#include "WorldState.h"

namespace TGX
{
Controller::Controller()
{
	Log::Success("Controller System Created");
	SetupBindings();
}

Controller &Controller::GetInstance()
{
	static Controller instance;
	return instance;
}

void Controller::SetupBindings()
{
	buttonBindings[ControllerAction::Select] = 0;
	buttonBindings[ControllerAction::Action] = 1;
	buttonBindings[ControllerAction::ToggleGrid] = 2;
	buttonBindings[ControllerAction::ToggleWaypoints] = 3;
	buttonBindings[ControllerAction::ToggleEconomy] = 5;
	buttonBindings[ControllerAction::Confirm] = 4;
	buttonBindings[ControllerAction::Exit] = 7;
}

void Controller::Update()
{
	previousState = currentState;

	if (!sf::Joystick::isConnected(controllerID))
	{
		return;
	}

	for (auto &binding : buttonBindings)
	{
		ControllerAction action = binding.first;
		unsigned int button = binding.second;

		currentState[action] = sf::Joystick::isButtonPressed(controllerID, button);
	}

	if (IsActionPressed(ControllerAction::ToggleGrid))
	{
		Debug::showGrid = !Debug::showGrid;
	}

	if (IsActionPressed(ControllerAction::ToggleWaypoints))
	{
		Debug::showWayPoints = !Debug::showWayPoints;
	}

	if (IsActionPressed(ControllerAction::ToggleEconomy))
	{
		Debug::showEconomy = !Debug::showEconomy;
	}

	if (IsActionPressed(ControllerAction::Exit))
	{
		Log::Success("Exit pressed - exiting game.");
		WorldState &world = WorldState::GetInstance();
		world.SetClosed(true);
	}
}

bool Controller::IsActionPressed(ControllerAction action)
{
	return currentState[action] && !previousState[action];
}

bool Controller::IsActionHeld(ControllerAction action)
{
	return currentState[action];
}

float Controller::GetAxis(sf::Joystick::Axis axis)
{
	if (!sf::Joystick::isConnected(controllerID))
	{
		return 0.f;
	}

	float value = sf::Joystick::getAxisPosition(controllerID, axis);

	if (std::abs(value) < deadzone)
	{
		return 0.f;
	}

	return value / 100.f;
}
} // namespace TGX
