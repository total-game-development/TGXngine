#pragma once

#include <SFML/Graphics.hpp>
#include <effolkronium/random.hpp>
#include <Common.hpp>
#include "Globals.h"
#include "InfantryStates.h"
#include "ItemInstance.h"
#include "Orders.h"
#include "PhysicsDefinitions.h"
#include "Window.h"

namespace TGX
{
using Random = effolkronium::random_static;

ItemInstance *globalItem;

Unique<Boundary> near;

Map<String, sf::Texture *> textureMap;

static Map<Orders::Order, Function<void(ItemInstance *)>> orderMap;

unsigned int width;
unsigned int height;

class Infantries
{
private:
	Unique<sf::CircleShape> selectionCircle;
	Unique<sf::RectangleShape> lifeBar;

public:
	Vector<std::tuple<int, int>> path;
	int circleIndex = 0;
	int circleCounter = 0;
	int animationCount = 1;
	int animationSpeed = 0;
	int animationLimit = 2;
	int animationSpeedLimit = 15;
	bool attacking = false;
	int reloadTimeLeft = 0;

	Infantries(ItemInstance *itemState)
	{
		CreateSelectionCircle(itemState->GetRadius());

		CreateLifeBar();
	}

	~Infantries() = default;

	void Draw(ItemInstance *itemState, std::vector<sf::Sprite *> *spritesRef)
	{
		Window &window = Window::GetInstance();

		if (itemState->IsSelected())
		{
			WorldState &world = WorldState::GetInstance();
			selectionCircle->setPosition(
				static_cast<float>((itemState->GetX() * 20.0) + world.GetMapXOffset() - 10),
				static_cast<float>((itemState->GetY() * 20.0) + world.GetMapYOffset()) - 10);

			window.Draw(*selectionCircle);
		}

		window.Draw(*lifeBar);

		window.Draw(*(*spritesRef)[itemState->GetFrame()]);

		auto *infantry = static_cast<InfantryState *>(itemState);

		sf::ConvexShape polygonShape;
		polygonShape.setPointCount(4);

		polygonShape.setPoint(0, sf::Vector2f(infantry->polygon[0], infantry->polygon[1]));
		polygonShape.setPoint(1, sf::Vector2f(infantry->polygon[2], infantry->polygon[3]));
		polygonShape.setPoint(2, sf::Vector2f(infantry->polygon[4], infantry->polygon[5]));
		polygonShape.setPoint(3, sf::Vector2f(infantry->polygon[6], infantry->polygon[7]));

		polygonShape.setFillColor(sf::Color::Transparent);
		polygonShape.setOutlineColor(sf::Color::Blue);
		polygonShape.setOutlineThickness(1.f);
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
			((itemState->GetX() * 20.0f) + static_cast<float>(world.GetMapXOffset() - 12)),
			((itemState->GetY() * 20.0f) + static_cast<float>(world.GetMapYOffset() - 25)));

		(*spritesRef)[itemState->GetFrame()]->setPosition(
			sf::Vector2f(
				static_cast<float>((itemState->GetX() * 20.0) + world.GetMapXOffset()),
				static_cast<float>((itemState->GetY() * 20.0) + world.GetMapYOffset())));

		auto index = static_cast<size_t>(
			WrapDirection(
				std::round(itemState->GetDirection()),
				itemState->GetDirections()));

		float cosAngle = InfantryState::cosDirectionAngles[index];
		float sinAngle = InfantryState::sinDirectionAngles[index];

		auto textureWidthHalf = static_cast<float>(itemState->GetWidth() / 2.0);
		auto textureHeightHalf = static_cast<float>(itemState->GetHeight() / 2.0);

		auto translateX = static_cast<float>(Globals::grid_size * itemState->GetX());
		auto translateY = static_cast<float>((itemState->GetY() * Globals::grid_size) + 80.0);

		auto *infantry = static_cast<InfantryState *>(itemState);

		infantry->polygon[0] = (-textureWidthHalf * cosAngle * 1.25f) - (-textureHeightHalf * sinAngle * 1.25f) + translateX;
		infantry->polygon[1] = (-textureWidthHalf * sinAngle * 1.25f) + (-textureHeightHalf * cosAngle * 1.25f) + translateY;
		infantry->polygon[2] = (textureWidthHalf * cosAngle * 1.25f) - (-textureHeightHalf * sinAngle * 1.25f) + translateX;
		infantry->polygon[3] = (textureWidthHalf * sinAngle * 1.25f) + (-textureHeightHalf * cosAngle * 1.25f) + translateY;

		infantry->polygon[4] = (textureWidthHalf * cosAngle * 1.25f) - (textureHeightHalf * sinAngle * 1.25f) + translateX;
		infantry->polygon[5] = (textureWidthHalf * sinAngle * 1.25f) + (textureHeightHalf * cosAngle * 1.25f) + translateY;
		infantry->polygon[6] = (-textureWidthHalf * cosAngle * 1.25f) - (textureHeightHalf * sinAngle * 1.25f) + translateX;
		infantry->polygon[7] = (-textureWidthHalf * sinAngle * 1.25f) + (textureHeightHalf * cosAngle * 1.25f) + translateY;
	}

private:
	void CreateSelectionCircle(float radius)
	{
		selectionCircle = std::make_unique<sf::CircleShape>(radius);
		selectionCircle->setFillColor(sf::Color(255, 216, 0, 51));
		selectionCircle->setOutlineColor(sf::Color(255, 255, 0, 128));
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

Map<int, Unique<Infantries>> infantries;

void Attack(ItemInstance *itemInstance);
void Action(ItemInstance *itemInstance);
void Move(ItemInstance *itemInstance);
void MoveTo(ItemInstance *itemInstance);
void Turning(InfantryState *);
void TurnToFire(InfantryState *);
void Firing(InfantryState *);
void Fire(InfantryState *);
void Moving(InfantryState *);
void Attacking(InfantryState *);
void Attacked(InfantryState *);
void Search(InfantryState *);
void SearchAndDestroy(InfantryState *);
void Standing(InfantryState *);
void Stand();
void Animate(InfantryState *);
void CreatePolygons();
void RegisterToQuadTree(const Set<String> &groups);
void AnimatePolygons(ItemInstance *itemInstance);
void SetPath(InfantryState *, float, float);
void SetTacticalCoordinates(ItemInstance *itemInstance, Vector<Point> path, float toX, float toY, int sight);
void Steering(ItemInstance *itemInstance);
void SpreadDestination(float &, float &);
void TestSearch();
void Destroyed(ItemInstance *itemInstance);
void Load(InfantryState *itemInstance);
void Unload(InfantryState *itemInstance);

Vector<ItemInstance *> Detect(ItemInstance *itemInstance, const Vector<ItemInstance *> &nearByItems);
} // namespace TGX
