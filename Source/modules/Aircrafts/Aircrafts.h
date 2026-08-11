#pragma once

#include <SFML/Graphics.hpp>
#include <Common.hpp>
#include "AircraftStates.h"
#include "BuildingStates.h"
#include "Globals.h"
#include "ItemInstance.h"
#include "Orders.h"
#include "Point.h"
#include "Window.h"

namespace TGX
{
ItemInstance *globalItem;

static Map<Orders::Order, Function<void(ItemInstance *)>> orderMap;

inline void Heading(const ItemInstance *itemInstance, float &headingX, float &headingY)
{
	headingX = 0.0f;
	headingY = 0.0f;

	const int directions = itemInstance->GetDirections();
	const auto tableSize = static_cast<int>(AircraftState::cosDirectionAngles.size());

	if (directions <= 0 || directions > tableSize)
	{
		return;
	}

	const float wrapped = WrapDirection(itemInstance->GetDirection(), directions);
	const float floored = std::floor(wrapped);

	auto lower = static_cast<size_t>(floored);
	float fraction = wrapped - floored;

	if (lower >= static_cast<size_t>(directions))
	{
		lower = 0;
		fraction = 0.0f;
	}

	size_t upper = lower + 1;

	if (upper >= static_cast<size_t>(directions))
	{
		upper = 0;
	}

	const float x = AircraftState::sinDirectionAngles[lower] +
					((AircraftState::sinDirectionAngles[upper] - AircraftState::sinDirectionAngles[lower]) * fraction);
	const float y = AircraftState::cosDirectionAngles[lower] +
					((AircraftState::cosDirectionAngles[upper] - AircraftState::cosDirectionAngles[lower]) * fraction);

	const float length = std::sqrt((x * x) + (y * y));

	if (length <= 0.0f)
	{
		return;
	}

	headingX = x / length;
	headingY = y / length;
}

class Aircrafts
{
private:
	Unique<sf::CircleShape> selectionCircle;
	Unique<sf::RectangleShape> lifeBar;

public:
	Orders *orders = nullptr;
	int animationOffset = 0;
	int animationCount = 1;
	int animationSpeed = 0;

	Aircrafts(ItemInstance *itemState)
	{
		CreateSelectionCircle(itemState->GetRadius());
		CreateLifeBar();

		orders = new Orders();
	}

	~Aircrafts()
	{
		delete orders;
	}

	void Draw(ItemInstance *itemState, std::vector<sf::Sprite *> *spritesRef)
	{
		Window &window = Window::GetInstance();

		if (itemState->IsSelected())
		{
			window.Draw(*selectionCircle);
			window.Draw(*lifeBar);
		}

		window.Draw(*(*spritesRef)[itemState->GetFrame()]);
	}

	void Update(ItemInstance *itemState, std::vector<sf::Sprite *> *spritesRef)
	{
		WorldState &world = WorldState::GetInstance();

		float maxHP = itemState->GetHitPoints();
		float currentHP = itemState->GetLife();
		float healthRatio = (maxHP > 0.0f) ? (currentHP / maxHP) : 0.0f;

		healthRatio = std::max(0.0f, std::min(1.0f, healthRatio));

		float maxWidth = 20.0f;
		lifeBar->setSize(sf::Vector2f(maxWidth * healthRatio, 4.0f));

		if (healthRatio > 0.5f)
		{
			lifeBar->setFillColor(sf::Color(0, 216, 0, 255));
		}
		else if (healthRatio > 0.2f)
		{
			lifeBar->setFillColor(sf::Color(255, 140, 0, 255));
		}
		else
		{
			lifeBar->setFillColor(sf::Color(216, 0, 0, 255));
		}

		lifeBar->setPosition(
			((itemState->GetX() * Globals::grid_size) + static_cast<float>(world.GetMapXOffset() - 12)),
			((itemState->GetY() * Globals::grid_size) + static_cast<float>(world.GetMapYOffset() - 30)));

		auto xPosition = ((itemState->GetX() * Globals::grid_size) + static_cast<float>(world.GetMapXOffset()));
		auto yPosition = ((itemState->GetY() * Globals::grid_size) + static_cast<float>(world.GetMapYOffset()));

		selectionCircle->setPosition(xPosition, yPosition);

		(*spritesRef)[itemState->GetFrame()]->setPosition(
			sf::Vector2f(xPosition, yPosition));
	}

private:
	void CreateSelectionCircle(float radius)
	{
		selectionCircle = std::make_unique<sf::CircleShape>(radius);
		selectionCircle->setFillColor(sf::Color(255, 216, 0, 51));
		selectionCircle->setOutlineColor(sf::Color(255, 255, 0, 128));
		selectionCircle->setOrigin(radius, radius);
		selectionCircle->setOutlineThickness(1);
	}

	void CreateLifeBar()
	{
		lifeBar = std::make_unique<sf::RectangleShape>(sf::Vector2f(20, 4));
		lifeBar->setFillColor(sf::Color(0, 216, 0, 255));
		lifeBar->setOutlineColor(sf::Color(0, 0, 0, 255));
		lifeBar->setOutlineThickness(1);
	}
};

Map<int, Unique<Aircrafts>> aircrafts;

AirportState *GetAirport(int uid);
void ReleaseHanger(AircraftState *itemInstance);

int RunwayForHanger(int hangerIndex);
bool ClaimRunway(AirportState *airport, int hangerIndex, int uid);
void ReleaseRunway(AirportState *airport, int uid);
bool TaxiClear(const AircraftState *itemInstance, const AirportState *airport);

void Action(ItemInstance *itemInstance);

void Move(ItemInstance *itemInstance);
void MoveTo(ItemInstance *itemInstance);
void Moving(AircraftState *itemInstance);

void Stand();
void Standing(AircraftState *itemInstance);

void TakeOff(AircraftState *itemInstance);
void TakingOff(AircraftState *itemInstance);
void Fly(AircraftState *itemInstance);
void Circle(AircraftState *itemInstance);

void Approach(AircraftState *itemInstance);
void Hold(AircraftState *itemInstance);
void Approaching(AircraftState *itemInstance);
void Land(AircraftState *itemInstance);
void Landing(AircraftState *itemInstance);

void Attack(ItemInstance *itemInstance);
void Attacked(AircraftState *itemInstance);
void Firing(AircraftState *itemInstance);
void Fire(AircraftState *itemInstance);
void BreakAway(AircraftState *itemInstance);
void BreakingAway(AircraftState *itemInstance);

void NoTarget(AircraftState *itemInstance);
void EnterCircle(AircraftState *itemInstance);

void Destroyed(ItemInstance *itemInstance);

void Animate(AircraftState *itemInstance);
void RegisterToQuadTree(const Set<String> &groups);
} // namespace TGX
