#pragma once

#include <SFML/Graphics.hpp>
#include <Common.hpp>
#include "ItemInstance.h"
#include "Orders.h"
#include "TurretStates.h"
#include "Window.h"

namespace TGX
{
ItemInstance *globalItem;

static Map<Orders::Order, Function<void(ItemInstance *)>> orderMap;

class Turrets
{
private:
	sf::RectangleShape *selectionRectangle;

public:
	Turrets(ItemInstance *itemInstance)
	{
		CreateSelectionRectangle(itemInstance->GetWidth(), itemInstance->GetHeight());
	}

	~Turrets()
	{
		delete selectionRectangle;
	}

	Turrets(const Turrets &) = delete;
	Turrets &operator=(const Turrets &) = delete;

	void Draw(ItemInstance *itemInstance, Vector<sf::Sprite *> *spritesRef)
	{
		Window &window = Window::GetInstance();

		if (itemInstance->IsSelected())
		{
			window.Draw(*selectionRectangle);
		}

		int frame = itemInstance->GetFrame();

		if (spritesRef == nullptr || frame < 0 || frame >= static_cast<int>(spritesRef->size()))
		{
			return;
		}

		window.Draw(*(*spritesRef)[frame]);
	}

	void Update(ItemInstance *itemState, Vector<sf::Sprite *> * /*spritesRef*/)
	{
		WorldState &world = WorldState::GetInstance();
		selectionRectangle->setPosition(
			((itemState->GetX() * 20.0f) + static_cast<float>(world.GetMapXOffset())),
			((itemState->GetY() * 20.0f) + static_cast<float>(world.GetMapYOffset())));
	}

private:
	void CreateSelectionRectangle(int width, int height)
	{
		selectionRectangle = new sf::RectangleShape();
		selectionRectangle->setSize(sf::Vector2f(static_cast<float>(width), static_cast<float>(height)));
		selectionRectangle->setFillColor(sf::Color(255, 216, 0, 51));
		selectionRectangle->setOutlineColor(sf::Color(255, 255, 0, 128));
		selectionRectangle->setOutlineThickness(1);
	}
};

using FNPTR_INIT = void (*)();

Map<int, Unique<Turrets>> turrets;
} // namespace TGX
