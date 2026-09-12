#include "Mouse.h"
#include "Globals.h"
#include "Logs.h"
#include "Window.h"
#include "WorldState.h"

#include <cmath>

namespace TGX
{

Mouse::Mouse()
{
	selectionBox = sf::RectangleShape(sf::Vector2f(0, 0));
	selectionBox.setFillColor(sf::Color::Transparent);
	selectionBox.setOutlineColor(sf::Color(255, 255, 255));
	selectionBox.setOutlineThickness(1);

	orders = std::make_unique<Orders>();

	Log::Success("Mouse Created");
}

Mouse &Mouse::GetInstance()
{
	static Mouse mouse;
	return mouse;
}

void Mouse::Suppress(bool inSuppressed)
{
	suppressed = inSuppressed;

	if (!suppressed)
	{
		return;
	}

	selectionBox.setPosition(0, 0);
	selectionBox.setSize(sf::Vector2f(0, 0));

	dragPressed = false;
	dragSelect = false;
	selectGameItems = false;
}

void Mouse::Click()
{
	if (suppressed)
	{
		return;
	}

	Log::Info("Mouse Clicked");

	dragPressed = true;
	selectGameItems = false;

	dragX = gameX;
	dragY = gameY;
}

void Mouse::RightClick()
{
	if (suppressed)
	{
		return;
	}

	WorldState &world = WorldState::GetInstance();

	if (world.IsEnemyItemUnderCursor() || world.IsResourceUnderCursor() || world.IsLoadableItemUnderCursor() || world.IsItemUnderCursor())
	{
		orders->order = Orders::Order::Action;
	}
	else
	{
		orders->order = Orders::Order::Move;
	}

	orders->toX = gameX / Globals::grid_size;
	orders->toY = (gameY - Globals::main_interface_height) / Globals::grid_size;
}

void Mouse::Release()
{
	if (suppressed)
	{
		return;
	}

	Log::Info("Mouse Released");

	selectionBox.setPosition(0, 0);
	selectionBox.setSize(sf::Vector2f(0, 0));

	if (dragSelect)
	{
		selectGameItems = true;
	}

	dragPressed = false;
	dragSelect = false;
}

void Mouse::Moved(float inX, float inY)
{
	if (!suppressed && dragPressed)
	{
		if ((std::abs(dragX - inX) > 4 || std::abs(dragY - inY) > 4))
		{
			dragSelect = true;
		}
	}

	x = inX;
	y = inY;
}

void Mouse::CalculateGameCoordinates()
{
	WorldState &world = WorldState::GetInstance();

	gameX = x + Globals::mapOffsetX;
	gameY = y + Globals::mapOffsetY;

	world.SetGameX(gameX);
	world.SetGameY(gameY);

	world.SetMouseX(x);
	world.SetMouseY(y);
}

void Mouse::Update()
{
	CalculateGameCoordinates();
}

void Mouse::Draw()
{
	if (suppressed)
	{
		return;
	}

	Window &window = Window::GetInstance();

	if (dragSelect)
	{
		selectionBoxX = std::min(gameX, dragX);
		selectionBoxY = std::min(gameY, dragY);

		selectionBoxWidth = std::abs(gameX - dragX);
		selectionBoxHeight = std::abs(gameY - dragY);

		selectionBox.setPosition(
			selectionBoxX - Globals::mapOffsetX,
			selectionBoxY - Globals::mapOffsetY);

		selectionBox.setSize(sf::Vector2f(selectionBoxWidth, selectionBoxHeight));

		window.Draw(selectionBox);
	}
}

void Mouse::DrawCursor(Vector<sf::Sprite> cursor)
{
	WorldState &world = WorldState::GetInstance();
	Window &window = Window::GetInstance();

	size_t mouseIndex = 0;

	if (world.IsItemUnderCursor())
	{
		mouseIndex = 1;
	}
	else if (world.IsEnemyItemUnderCursor())
	{
		mouseIndex = 2;
	}
	else if (world.IsResourceUnderCursor())
	{
		mouseIndex = 3;
	}
	else if (world.IsLoadableItemUnderCursor())
	{
		mouseIndex = 4;
	}

	sf::Vector2f mousePosition(x, y);

	sf::Sprite sprite = cursor.at(mouseIndex);
	sprite.setPosition(mousePosition);

	window.Draw(sprite);
}

Orders *Mouse::CurrentOrder()
{
	return orders.get();
}

bool Mouse::IsSelectGameItems() const
{
	return selectGameItems;
}

float Mouse::GetSelectionBoxX() const
{
	return selectionBoxX;
}

float Mouse::GetSelectionBoxY() const
{
	return selectionBoxY;
}

float Mouse::GetSelectionBoxWidth() const
{
	return selectionBoxWidth;
}

float Mouse::GetSelectionBoxHeight() const
{
	return selectionBoxHeight;
}

} // namespace TGX
