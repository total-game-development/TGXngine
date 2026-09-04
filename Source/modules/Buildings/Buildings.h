#pragma once

#include <SFML/Graphics.hpp>
#include <Common.hpp>
#include <algorithm>
#include "ItemInstance.h"
#include "Window.h"

namespace TGX
{
ItemInstance *globalItem;
Map<String, Function<ItemInstance *()>> buildingStateRegistry;

struct Building
{
	String name;

	float radius = 0;
	int frames = 0;
	int speed = 0;
	int turnSpeed = 0;
	int topSpeed = 0;
	int passableGridWidth = 0;
	int passableGridHeight = 0;
	bool animation = false;
	int animationCount = 0;
	int animationLimit = 0;
	int animationSpeed = 0;
	int animationSpeedLimit = 0;
	int animationSlowSpeed = 0;
	int animationSlowSpeedLimit = 0;
	int directions = 0;
};

class Buildings
{
private:
	sf::RectangleShape *selectionRectangle;
	sf::RectangleShape *lifeBar;
	float lifeBarWidth = 0.0f;
	bool showLifeBar = false;

public:
	Buildings(ItemInstance *itemInstance)
	{
		CreateSelectionRectangle(itemInstance->GetX(), itemInstance->GetY(), itemInstance->GetWidth(), itemInstance->GetHeight());
		CreateLifeBar(itemInstance->GetWidth());
	}

	~Buildings()
	{
		delete selectionRectangle;
		delete lifeBar;
	}

	void Draw(ItemInstance *itemInstance, Vector<sf::Sprite *> *spritesRef)
	{
		Window &window = Window::GetInstance();

		if (itemInstance->IsSelected())
		{
			window.Draw(*selectionRectangle);
		}

		if (showLifeBar)
		{
			window.Draw(*lifeBar);
		}

		window.Draw(*(*spritesRef)[itemInstance->GetFrame()]);
	}

	void Update(ItemInstance *itemState, Vector<sf::Sprite *> * /*spritesRef*/)
	{
		WorldState &world = WorldState::GetInstance();
		selectionRectangle->setPosition(
			((itemState->GetX() * 20.0f) + static_cast<float>(world.GetMapXOffset())),
			((itemState->GetY() * 20.0f) + static_cast<float>(world.GetMapYOffset())));

		const float maxHitPoints = itemState->GetHitPoints();
		const float ratio = (maxHitPoints > 0.0f)
								? std::max(0.0f, std::min(1.0f, itemState->GetLife() / maxHitPoints))
								: 0.0f;

		showLifeBar = itemState->IsSelected() || ratio < 1.0f;

		if (!showLifeBar)
		{
			return;
		}

		const float footprint = (itemState->GetCenterX() - itemState->GetX()) * 2.0f * 20.0f;
		const float barWidth = (footprint > 0.0f) ? footprint : lifeBarWidth;

		lifeBar->setSize(sf::Vector2f(barWidth * ratio, 4.0f));

		if (ratio > 0.5f)
		{
			lifeBar->setFillColor(sf::Color(0, 216, 0, 255));
		}
		else if (ratio > 0.2f)
		{
			lifeBar->setFillColor(sf::Color(255, 140, 0, 255));
		}
		else
		{
			lifeBar->setFillColor(sf::Color(216, 0, 0, 255));
		}

		lifeBar->setPosition(
			((itemState->GetCenterX() * 20.0f) + static_cast<float>(world.GetMapXOffset()) - (barWidth / 2.0f)),
			((itemState->GetY() * 20.0f) + static_cast<float>(world.GetMapYOffset()) - 8.0f));
	}

private:
	void CreateSelectionRectangle(float /*x*/, float /*y*/, int width, int height)
	{
		selectionRectangle = new sf::RectangleShape();
		selectionRectangle->setSize(sf::Vector2f(static_cast<float>(width), static_cast<float>(height)));
		selectionRectangle->setFillColor(sf::Color(255, 216, 0, 51));
		selectionRectangle->setOutlineColor(sf::Color(255, 255, 0, 128));
		selectionRectangle->setOutlineThickness(1);
	}

	void CreateLifeBar(int width)
	{
		lifeBarWidth = static_cast<float>(width);

		lifeBar = new sf::RectangleShape(sf::Vector2f(lifeBarWidth, 4.0f));
		lifeBar->setFillColor(sf::Color(0, 216, 0, 255));
		lifeBar->setOutlineColor(sf::Color(0, 0, 0, 255));
		lifeBar->setOutlineThickness(1);
	}
};

using FNPTR_INIT = void (*)();

Map<int, Buildings *> buildings;
} // namespace TGX
