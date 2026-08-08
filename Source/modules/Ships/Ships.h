#pragma once

#include <SFML/Graphics.hpp>
#include <effolkronium/random.hpp>
#include <Common.hpp>
#include "Globals.h"
#include "ItemInstance.h"
#include "NextStep.h"
#include "Orders.h"
#include "Point.h"
#include "ShipStates.h"
#include "Window.h"

namespace TGX
{
using Random = effolkronium::random_static;

ItemInstance *globalItem;

static Map<Orders::Order, Function<void(ItemInstance *)>> orderMap;

class Ships
{
private:
	Unique<sf::CircleShape> selectionCircle;
	Unique<sf::RectangleShape> lifeBar;
	sf::ConvexShape bodyPolygon;
	sf::ConvexShape skinPolygon;
	sf::ConvexShape bumperPolygon;
	sf::ConvexShape visionPolygon;

public:
	Orders *orders = nullptr;
	NextStep *nextStep = nullptr;
	int circleIndex = 0;
	int circleCounter = 0;
	int animationOffset = 0;
	int animationCount = 1;
	int animationSpeed = 0;

	Ships(ItemInstance *itemState)
	{
		CreateSelectionCircle(itemState->GetRadius());
		CreateLifeBar();

		orders = new Orders();
	}

	~Ships()
	{
		delete orders;
		delete nextStep;
	}

	void Draw(ItemInstance *itemState, std::vector<sf::Sprite *> *spritesRef)
	{
		Window &window = Window::GetInstance();

		if (itemState->IsSelected())
		{
			window.Draw(*selectionCircle);
		}

		window.Draw(*lifeBar);

		window.Draw(*(*spritesRef)[itemState->GetFrame()]);

		auto *ship = static_cast<ShipState *>(itemState);

		sf::ConvexShape polygonShape;
		polygonShape.setPointCount(4);

		polygonShape.setPoint(0, sf::Vector2f(ship->polygon[0], ship->polygon[1]));
		polygonShape.setPoint(1, sf::Vector2f(ship->polygon[2], ship->polygon[3]));
		polygonShape.setPoint(2, sf::Vector2f(ship->polygon[4], ship->polygon[5]));
		polygonShape.setPoint(3, sf::Vector2f(ship->polygon[6], ship->polygon[7]));

		polygonShape.setFillColor(sf::Color::Transparent);
		polygonShape.setOutlineColor(sf::Color::Red);
		polygonShape.setOutlineThickness(1.f);
	}

	void Update(ItemInstance *itemState, std::vector<sf::Sprite *> *spritesRef)
	{
		WorldState &world = WorldState::GetInstance();
		lifeBar->setPosition(
			((itemState->GetX() * 20.0f) + static_cast<float>(world.GetMapXOffset() - 12)),
			((itemState->GetY() * 20.0f) + static_cast<float>(world.GetMapYOffset() - 30)));

		auto xPosition = ((itemState->GetX() * 20.0f) + static_cast<float>(world.GetMapXOffset()));
		auto yPosition = ((itemState->GetY() * 20.0f) + static_cast<float>(world.GetMapYOffset()));

		selectionCircle->setPosition(xPosition, yPosition);

		(*spritesRef)[itemState->GetFrame()]->setPosition(
			sf::Vector2f(xPosition, yPosition));

		auto index = static_cast<size_t>(WrapDirection(std::round(itemState->GetDirection()), itemState->GetDirections()));

		float cosAngle = ShipState::cosDirectionAngles[index];
		float sinAngle = ShipState::sinDirectionAngles[index];

		auto textureWidthHalf = static_cast<float>(itemState->GetWidth() / 2.0);
		auto textureHeightHalf = static_cast<float>(itemState->GetHeight() / 2.0);

		auto translateX = static_cast<float>(itemState->GetX() * Globals::grid_size);
		auto translateY = static_cast<float>((itemState->GetY() * Globals::grid_size) + 80.0);

		static_cast<ShipState *>(itemState)->polygon[0] = (-textureWidthHalf * cosAngle * 1.25f) - (-textureHeightHalf * sinAngle * 1.25f) + translateX;
		static_cast<ShipState *>(itemState)->polygon[1] = (-textureWidthHalf * sinAngle * 1.25f) + (-textureHeightHalf * cosAngle * 1.25f) + translateY;
		static_cast<ShipState *>(itemState)->polygon[2] = (textureWidthHalf * cosAngle * 1.25f) - (-textureHeightHalf * sinAngle * 1.25f) + translateX;
		static_cast<ShipState *>(itemState)->polygon[3] = (textureWidthHalf * sinAngle * 1.25f) + (-textureHeightHalf * cosAngle * 1.25f) + translateY;

		static_cast<ShipState *>(itemState)->polygon[4] = (textureWidthHalf * cosAngle * 1.25f) - (textureHeightHalf * sinAngle * 1.25f) + translateX;
		static_cast<ShipState *>(itemState)->polygon[5] = (textureWidthHalf * sinAngle * 1.25f) + (textureHeightHalf * cosAngle * 1.25f) + translateY;
		static_cast<ShipState *>(itemState)->polygon[6] = (-textureWidthHalf * cosAngle * 1.25f) - (textureHeightHalf * sinAngle * 1.25f) + translateX;
		static_cast<ShipState *>(itemState)->polygon[7] = (-textureWidthHalf * sinAngle * 1.25f) + (textureHeightHalf * cosAngle * 1.25f) + translateY;
	}

	float WrapDirection(float direction, int directions)
	{
		while (direction < 0)
		{
			direction += static_cast<float>(directions);
		}
		while (direction >= static_cast<float>(directions))
		{
			direction -= static_cast<float>(directions);
		}

		return direction;
	}

private:
	void CreateSelectionCircle(float radius)
	{
		selectionCircle = std::make_unique<sf::CircleShape>(radius);
		selectionCircle->setFillColor(sf::Color(255, 216, 0, 51));
		selectionCircle->setOutlineColor(sf::Color(255, 255, 0, 128));
		selectionCircle->setOrigin(
			radius,
			radius);
		selectionCircle->setOutlineThickness(1);
	}

	void CreateLifeBar()
	{
		lifeBar = std::make_unique<sf::RectangleShape>(sf::Vector2f(20, 4));
		lifeBar->setFillColor(sf::Color(0, 216, 0, 255));
		lifeBar->setOutlineColor(sf::Color(0, 0, 0, 255));
		lifeBar->setOutlineThickness(1);
	}

	sf::ConvexShape CreateVisualOutline(std::array<float, 16> array, sf::Color colour)
	{
		sf::ConvexShape outline;
		outline.setPointCount(4);
		outline.setPoint(0, sf::Vector2f(array[0], array[1]));
		outline.setPoint(1, sf::Vector2f(array[2], array[3]));
		outline.setPoint(2, sf::Vector2f(array[4], array[5]));
		outline.setPoint(3, sf::Vector2f(array[6], array[7]));
		outline.setFillColor(sf::Color::Transparent);
		outline.setOutlineColor(colour);
		outline.setOutlineThickness(1.f);

		return outline;
	}
};

Map<int, Unique<Ships>> ships;

using FNPTR_INIT = void (*)();

void InitTruck();

void Action(ItemInstance *itemInstance);

void Move(ItemInstance *itemInstance);
void MoveTo(ItemInstance *itemInstance);
void Turning(ShipState *);
void Moving(ShipState *);

void Standing(ShipState *);
void Stand();

void Animate(ShipState *);
void RegisterToQuadTree(const Set<String> &groups);

void TestSearch();
void TestPhysics();

void Steering(ItemInstance *itemInstance);
void SetPath(ShipState *itemInstance, float toX, float toY);

void Stop(ItemInstance *item);
void Velocity(ItemInstance *item);

void Attack(ItemInstance *itemInstance);
void TurnToFire(ShipState *itemInstance);
void Firing(ShipState *itemInstance);
void Fire(ShipState *itemInstance);
void Destroyed(ItemInstance *itemInstance);
void Unload(ItemInstance *itemInstance);

void Extract(ItemInstance *itemInstance);

void OnPath(const Vector<Point> &path);
void SetTacticalCoordinates(ItemInstance *itemInstance, Vector<Point> path, float toX, float toY, int sight);

Vector<ItemInstance *> Detect(ItemInstance *itemInstance, const Vector<ItemInstance *> &nearByItems);
} // namespace TGX
