#pragma once

#include <SFML/Window/Joystick.hpp>
#include "Core.h"

namespace TGX
{
enum class ControllerAction
{
	Confirm,
	Action,
	ToggleGrid,
	ToggleWaypoints,
	ToggleEconomy,
	Select,
	Exit
};

class Controller
{
private:
	Controller();
	~Controller() = default;

public:
	static Controller &GetInstance();

	Controller(const Controller &) = delete;
	Controller &operator=(const Controller &) = delete;

	Controller(Controller &&) = delete;
	Controller &operator=(Controller &&) = delete;

	void Update();

	bool IsActionPressed(ControllerAction action);
	bool IsActionHeld(ControllerAction action);

	float GetAxis(sf::Joystick::Axis axis);

private:
	void SetupBindings();

private:
	unsigned int controllerID = 0;

	Map<ControllerAction, unsigned int> buttonBindings;

	Map<ControllerAction, bool> currentState;
	Map<ControllerAction, bool> previousState;

	float deadzone = 15.f;
};
} // namespace TGX
