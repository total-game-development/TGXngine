#include "Aircrafts.h"

#include <algorithm>
#include "Enums.h"
#include "ImageLoader.h"
#include "Logs.h"
#include "Lookup.h"
#include "Physics.h"
#include "StringUtils.hpp"
#include "module_interface.h"

namespace TGX
{
constexpr float movementScale = 1.0f / 96.0f;
constexpr float turnScale = 1.0f / 8.0f;
constexpr float reloadFramesPerSecond = 60.0f;
constexpr float arrivalThreshold = 0.01f;
constexpr float breakAwayDistance = 80.0f;

extern "C"
{
	MODULE_API void OutputTest()
	{
	}

	MODULE_API void Init()
	{
		if (orderMap.empty())
		{
			orderMap = {
				{Orders::Order::Action, [](ItemInstance *state) {
					 Action(state);
				 }},
				{Orders::Order::Move, [](ItemInstance *state) {
					 Move(state);
				 }},
				{Orders::Order::MoveTo, [](ItemInstance *state) {
					 MoveTo(state);
				 }},
				{Orders::Order::Moving, [](ItemInstance *state) {
					 Moving(static_cast<AircraftState *>(state));
				 }},
				{Orders::Order::Standing, [](ItemInstance *state) {
					 Standing(static_cast<AircraftState *>(state));
				 }},
				{Orders::Order::Stand, [](ItemInstance * /*state*/) {
					 Stand();
				 }},
				{Orders::Order::TakeOff, [](ItemInstance *state) {
					 TakeOff(static_cast<AircraftState *>(state));
				 }},
				{Orders::Order::TakingOff, [](ItemInstance *state) {
					 TakingOff(static_cast<AircraftState *>(state));
				 }},
				{Orders::Order::Fly, [](ItemInstance *state) {
					 Fly(static_cast<AircraftState *>(state));
				 }},
				{Orders::Order::Circle, [](ItemInstance *state) {
					 Circle(static_cast<AircraftState *>(state));
				 }},
				{Orders::Order::Approach, [](ItemInstance *state) {
					 Approach(static_cast<AircraftState *>(state));
				 }},
				{Orders::Order::Approaching, [](ItemInstance *state) {
					 Approaching(static_cast<AircraftState *>(state));
				 }},
				{Orders::Order::Land, [](ItemInstance *state) {
					 Land(static_cast<AircraftState *>(state));
				 }},
				{Orders::Order::Landing, [](ItemInstance *state) {
					 Landing(static_cast<AircraftState *>(state));
				 }},
				{Orders::Order::Attack, [](ItemInstance *state) {
					 Attack(state);
				 }},
				{Orders::Order::Attacked, [](ItemInstance *state) {
					 Attacked(static_cast<AircraftState *>(state));
				 }},
				{Orders::Order::Firing, [](ItemInstance *state) {
					 Firing(static_cast<AircraftState *>(state));
				 }},
				{Orders::Order::Fire, [](ItemInstance *state) {
					 Fire(static_cast<AircraftState *>(state));
				 }},
				{Orders::Order::BreakAway, [](ItemInstance *state) {
					 BreakAway(static_cast<AircraftState *>(state));
				 }},
				{Orders::Order::BreakingAway, [](ItemInstance *state) {
					 BreakingAway(static_cast<AircraftState *>(state));
				 }},
				{Orders::Order::Destroyed, [](ItemInstance *state) {
					 Destroyed(state);
				 }},
			};
		}

		Log::Success("AircraftState Init created");
	}

	MODULE_API ItemInstance *Awake(const String &name)
	{
		Log::Success("AircraftState Awake " + name);

		if (name == "apache")
		{
			return globalItem = new ApacheState();
		}
		if (name == "jet")
		{
			return globalItem = new JetState();
		}
		if (name == "bomber")
		{
			return globalItem = new BomberState();
		}
		if (name == "transport")
		{
			return globalItem = new TransportState();
		}

		return nullptr;
	}

	MODULE_API void Create(
		Vector<sf::Sprite *> *spritesRef,
		Vector<sf::Texture *> *texturesRef)
	{
		if (!globalItem)
		{
			Log::Error("Create() failed: itemInstance is null");
			return;
		}

		auto name = globalItem->GetName();

		aircrafts[globalItem->GetUid()] = std::make_unique<Aircrafts>(globalItem);

		WorldState &world = WorldState::GetInstance();

		auto *aircraft = static_cast<AircraftState *>(globalItem);

		if (globalItem->IsTraining())
		{
			AirportState *airport = GetAirport(world.primaryItems["airport"]);

			if (airport != nullptr)
			{
				int deployUid = airport->GetUid();

				if (aircraft->CanLandOnHelipad() && airport->helipadUid == INT_MIN)
				{
					airport->helipadUid = globalItem->GetUid();

					globalItem->SetX(airport->GetCenterX() + airport->helipadDeployPosition.x);
					globalItem->SetY(airport->GetCenterY() + airport->helipadDeployPosition.y);
					globalItem->SetDirection(static_cast<float>(airport->helipadDeployPosition.direction));

					aircraft->deployUid = deployUid;
					aircraft->currentHangerPosition = 0;
					aircraft->landingDirection = airport->helipadDeployPosition.direction;
				}
				else
				{
					auto deployIt = world.deployMap.find(deployUid);

					if (deployIt != world.deployMap.end())
					{
						auto &deploys = deployIt->second;

						size_t i = 0;

						for (; i < deploys.size(); i++)
						{
							if (std::get<2>(deploys[i]) == INT_MIN)
							{
								std::get<2>(deploys[i]) = globalItem->GetUid();
								break;
							}
						}

						if (i < deploys.size())
						{
							globalItem->SetX(airport->GetX() + std::get<0>(deploys[i]));
							globalItem->SetY(airport->GetY() + std::get<1>(deploys[i]));
							globalItem->SetDirection(static_cast<float>(airport->hangerPositions[i].direction));

							aircraft->deployUid = deployUid;
							aircraft->currentHangerPosition = static_cast<int>(i);
							aircraft->landingDirection = airport->hangerPositions[i].direction;
						}
					}
				}
			}
			else
			{
				Log::Error("Airport item not found!");
			}
		}
		else
		{
			aircraft->landed = false;
		}

		String asset = globalItem->GetTeam() + "/" + globalItem->GetType() + "/" + name;

		for (int i = 0; i < globalItem->GetFrames(); i++)
		{
			String filename = "Resources/images/" + asset + "/" + std::to_string(i) + ".png";

			ImageLoader::Load(filename, globalItem, spritesRef, texturesRef);
		}

		globalItem->SetLife(globalItem->GetHitPoints());
		globalItem->SetArmy(true);

		if (!texturesRef->empty())
		{
			globalItem->SetWidth(static_cast<int>((*texturesRef)[0]->getSize().x));
			globalItem->SetHeight(static_cast<int>((*texturesRef)[0]->getSize().y));
		}

		RegisterToQuadTree(globalItem->GetGroups());
		Log::Success("Aircraft " + name + " created successfully");
	}

	MODULE_API void SendOrders(ItemInstance *itemInstance)
	{
		auto orderFunction = orderMap.find(itemInstance->GetOrders()->order);

		if (orderFunction != orderMap.end())
		{
			orderFunction->second(itemInstance);
		}
	}

	MODULE_API void ProcessOrders(ItemInstance *itemInstance)
	{
		if (itemInstance->GetOrders()->order == Orders::Order::None)
		{
			return;
		}

		auto orderFunction = orderMap.find(itemInstance->GetOrders()->order);

		if (orderFunction != orderMap.end())
		{
			orderFunction->second(itemInstance);
		}
	}

	MODULE_API void Draw(ItemInstance *itemInstance, std::vector<sf::Sprite *> *spritesRef)
	{
		aircrafts[itemInstance->GetUid()]->Draw(itemInstance, spritesRef);
	}

	MODULE_API void Update(ItemInstance *itemInstance, std::vector<sf::Sprite *> *spritesRef)
	{
		if (itemInstance->GetHidden())
		{
			return;
		}

		WorldState &world = WorldState::GetInstance();

		auto *aircraft = static_cast<AircraftState *>(itemInstance);

		if (aircraft->reloadTimeLeft > 0.0f)
		{
			aircraft->reloadTimeLeft -= world.GetDeltaTime();
		}

		const bool isVisible = itemInstance->GetTeam() == world.GetTeam() || itemInstance->isVisible();

		if (HoverFromCenter(itemInstance, itemInstance->GetRadius()) && isVisible)
		{
			world.SetItemUidThatIsUnderCursor(itemInstance->GetUid());

			if (world.GetTeam() == itemInstance->GetTeam())
			{
				world.SetItemUnderCursor(true);

				if (world.IsLeftClicked())
				{
					world.selected.emplace_back(itemInstance->GetUid());
					Log::Success("Aircraft added to selection list (size: " + std::to_string(world.selected.size()) + ")");
				}
			}
			else
			{
				if (!world.selected.empty())
				{
					int index = LookUp::Get(world.selected[0]);

					if (index != -1)
					{
						ItemInstance *selectedItem = world.items[index].get();

						if (selectedItem && selectedItem->CanAttack())
						{
							world.SetEnemyItemUnderCursor(true);

							if (world.IsRightClicked())
							{
								for (int uid : world.selected)
								{
									int targetIndex = LookUp::Get(uid);

									if (targetIndex != -1)
									{
										ItemInstance *targetInstance = world.items[targetIndex].get();

										if (targetInstance)
										{
											targetInstance->SetTargetUid(itemInstance->GetUid());
										}
									}
								}
							}
						}
					}
				}
			}
		}

		int frame = itemInstance->GetFrame();

		if (spritesRef == nullptr || frame < 0 || frame >= static_cast<int>(spritesRef->size()))
		{
			return;
		}

		Animate(aircraft);

		int facing = static_cast<int>(WrapDirection(
			std::round(itemInstance->GetDirection()), itemInstance->GetDirections()));

		itemInstance->SetFrame(
			(facing + aircrafts[itemInstance->GetUid()]->animationOffset) % itemInstance->GetFrames());

		aircrafts[itemInstance->GetUid()]->Update(itemInstance, spritesRef);
	}

	MODULE_API void Delete(int uid)
	{
		auto it = aircrafts.find(uid);

		if (it != aircrafts.end())
		{
			aircrafts.erase(it);
		}

		Log::Clean("Aircraft deleted. Remaining aircrafts: " + std::to_string(aircrafts.size()));
	}

	MODULE_API void Destroy(const std::string &name)
	{
		Log::Clean("Destroy static aircraft { " + name + " }");
	}
}

AirportState *GetAirport(int uid)
{
	if (uid == INT_MIN)
	{
		return nullptr;
	}

	WorldState &world = WorldState::GetInstance();

	int index = LookUp::Get(uid);

	if (index == -1 || static_cast<size_t>(index) >= world.items.size())
	{
		return nullptr;
	}

	ItemInstance *item = world.items[index].get();

	if (item == nullptr || item->GetName() != "airport")
	{
		return nullptr;
	}

	return static_cast<AirportState *>(item);
}

void ReleaseHanger(AircraftState *itemInstance)
{
	AirportState *airport = GetAirport(itemInstance->deployUid);

	if (airport == nullptr)
	{
		return;
	}

	if (itemInstance->CanLandOnHelipad())
	{
		if (airport->helipadUid == itemInstance->GetUid())
		{
			airport->helipadUid = INT_MIN;
		}

		return;
	}

	WorldState &world = WorldState::GetInstance();

	auto deployIt = world.deployMap.find(airport->GetUid());

	if (deployIt == world.deployMap.end())
	{
		return;
	}

	for (auto &deploy : deployIt->second)
	{
		if (std::get<2>(deploy) == itemInstance->GetUid())
		{
			std::get<2>(deploy) = INT_MIN;
			return;
		}
	}
}

void Advance(AircraftState *itemInstance, float speed)
{
	WorldState &world = WorldState::GetInstance();

	float movement = speed * movementScale * world.GetDeltaTime();

	size_t index = DirectionTableIndex(itemInstance);

	float moveX = movement * AircraftState::sinDirectionAngles[index];
	float moveY = -movement * AircraftState::cosDirectionAngles[index];

	itemInstance->SetLastMovementX(moveX);
	itemInstance->SetLastMovementY(moveY);
	itemInstance->SetX(itemInstance->GetX() + moveX);
	itemInstance->SetY(itemInstance->GetY() + moveY);
}

float ArrivalThresholdSquared(const AircraftState *itemInstance)
{
	float threshold = itemInstance->GetRadius() / static_cast<float>(Globals::grid_size);

	return threshold * threshold;
}

float DistanceSquared(float fromX, float fromY, float toX, float toY)
{
	float dx = toX - fromX;
	float dy = toY - fromY;

	return (dx * dx) + (dy * dy);
}

void EnterCircle(AircraftState *itemInstance)
{
	itemInstance->circleIndex = static_cast<int>(WrapDirection(
		std::round(itemInstance->GetDirection()), itemInstance->GetDirections()));

	itemInstance->wayPointX = AircraftState::circlePaths[itemInstance->circleIndex].x + itemInstance->GetX();
	itemInstance->wayPointY = AircraftState::circlePaths[itemInstance->circleIndex].y + itemInstance->GetY();

	itemInstance->SetSpeed(AircraftState::circlePaths[itemInstance->circleIndex].speed);
	itemInstance->minDistance = AircraftState::farthestDistance;
}

void Action(ItemInstance *itemInstance)
{
	WorldState &world = WorldState::GetInstance();

	Orders::Order order = Orders::Order::Stand;

	if (world.IsEnemyItemUnderCursor())
	{
		order = Orders::Order::Attack;
	}
	else if (world.IsItemUnderCursor())
	{
		AirportState *airport = GetAirport(world.GetItemUidThatIsUnderCursor());

		if (airport != nullptr && airport->GetTeam() == itemInstance->GetTeam())
		{
			itemInstance->GetOrders()->target_uid = airport->GetUid();
			order = Orders::Order::Approach;
		}
	}

	itemInstance->SetOrders(order);
}

void Move(ItemInstance *itemInstance)
{
	auto *aircraft = static_cast<AircraftState *>(itemInstance);

	if (aircraft->takingOff)
	{
		itemInstance->SetOrders(Orders::Order::TakingOff);
		return;
	}

	if (aircraft->landed)
	{
		itemInstance->SetOrders(Orders::Order::TakeOff);
		return;
	}

	ReleaseHanger(aircraft);

	itemInstance->SetSpeed(aircraft->GetTopSpeed());
	itemInstance->SetState(ItemStates::Flying);

	aircraft->circleCounter = 0;
	aircraft->minDistance = AircraftState::farthestDistance;

	itemInstance->SetOrders(Orders::Order::MoveTo);
}

void MoveTo(ItemInstance *itemInstance)
{
	auto *aircraft = static_cast<AircraftState *>(itemInstance);

	WorldState &world = WorldState::GetInstance();

	if (itemInstance->GetState() == ItemStates::Attacking)
	{
		int targetIndex = LookUp::Get(itemInstance->GetTargetUid());

		if (targetIndex == -1)
		{
			NoTarget(aircraft);
			return;
		}

		itemInstance->GetOrders()->toX = world.items[targetIndex]->GetX();
		itemInstance->GetOrders()->toY = world.items[targetIndex]->GetY();
	}
	else
	{
		float distanceFromDestinationSquared = DistanceSquared(
			itemInstance->GetX(), itemInstance->GetY(),
			itemInstance->GetOrders()->toX, itemInstance->GetOrders()->toY);

		if (distanceFromDestinationSquared < ArrivalThresholdSquared(aircraft) ||
			distanceFromDestinationSquared > aircraft->minDistance)
		{
			EnterCircle(aircraft);
			itemInstance->SetOrders(Orders::Order::Circle);
			return;
		}

		aircraft->minDistance = distanceFromDestinationSquared;
	}

	Moving(aircraft);
}

void Moving(AircraftState *itemInstance)
{
	WorldState &world = WorldState::GetInstance();

	float newDirection = FindAngle(
		itemInstance->GetOrders()->toX, itemInstance->GetOrders()->toY,
		itemInstance->GetX(), itemInstance->GetY(),
		itemInstance->GetDirections());

	float difference = AngleDiff(
		itemInstance->GetDirection(),
		newDirection,
		itemInstance->GetDirections());

	float turnAmount = static_cast<float>(itemInstance->GetTurnSpeed()) * turnScale * world.GetDeltaTime();

	if (std::abs(difference) > turnAmount)
	{
		float direction = itemInstance->GetDirection() + (turnAmount * std::abs(difference) / difference);
		itemInstance->SetDirection(WrapDirection(direction, itemInstance->GetDirections()));
		return;
	}

	itemInstance->SetDirection(
		WrapDirection(itemInstance->GetDirection() + difference, itemInstance->GetDirections()));

	Advance(itemInstance, itemInstance->GetSpeed());

	if (itemInstance->GetState() != ItemStates::Attacking)
	{
		return;
	}

	int targetIndex = LookUp::Get(itemInstance->GetTargetUid());

	if (targetIndex == -1)
	{
		return;
	}

	float sight = static_cast<float>(itemInstance->GetSight());

	float distanceToTargetSquared = DistanceSquared(
		itemInstance->GetX(), itemInstance->GetY(),
		world.items[targetIndex]->GetX(), world.items[targetIndex]->GetY());

	if (distanceToTargetSquared < (sight * sight))
	{
		itemInstance->SetOrders(Orders::Order::Fire);
	}
}

void Stand()
{
}

void Standing(AircraftState *itemInstance)
{
	if (itemInstance->GetState() == ItemStates::Attacking)
	{
		itemInstance->SetOrders(Orders::Order::Firing);
		return;
	}

	itemInstance->SetOrders(Orders::Order::Stand);
}

void TakeOff(AircraftState *itemInstance)
{
	AirportState *airport = GetAirport(itemInstance->deployUid);

	if (airport == nullptr)
	{
		itemInstance->landed = false;
		itemInstance->SetOrders(Orders::Order::Fly);
		return;
	}

	if (itemInstance->CanLandOnHelipad())
	{
		if (airport->helipadUid == itemInstance->GetUid())
		{
			airport->helipadUid = INT_MIN;
		}

		itemInstance->landed = false;
		itemInstance->SetOrders(Orders::Order::Fly);
		return;
	}

	WorldState &world = WorldState::GetInstance();

	auto deployIt = world.deployMap.find(airport->GetUid());

	if (deployIt != world.deployMap.end())
	{
		auto &deploys = deployIt->second;

		for (size_t i = 0; i < deploys.size(); i++)
		{
			if (std::get<2>(deploys[i]) != itemInstance->GetUid())
			{
				continue;
			}

			std::get<2>(deploys[i]) = INT_MIN;

			itemInstance->currentHangerPosition = static_cast<int>(i);
			itemInstance->takingOffIndex = 0;
			itemInstance->previousDistance = AircraftState::farthestDistance;
			itemInstance->landed = false;
			itemInstance->takingOff = true;
			itemInstance->SetOrders(Orders::Order::TakingOff);
			return;
		}
	}

	itemInstance->landed = false;
	itemInstance->SetOrders(Orders::Order::Fly);
}

void TakingOff(AircraftState *itemInstance)
{
	AirportState *airport = GetAirport(itemInstance->deployUid);

	if (airport == nullptr)
	{
		itemInstance->landed = false;
		itemInstance->takingOff = false;
		itemInstance->SetOrders(Orders::Order::Fly);
		return;
	}

	if (itemInstance->currentHangerPosition < 0 ||
		static_cast<size_t>(itemInstance->currentHangerPosition) >= airport->takeOffPaths.size())
	{
		itemInstance->landed = false;
		itemInstance->takingOff = false;
		itemInstance->SetOrders(Orders::Order::Fly);
		return;
	}

	const Vector<AirWayPoint> &path = airport->takeOffPaths[itemInstance->currentHangerPosition];

	if (itemInstance->takingOffIndex < 0 ||
		static_cast<size_t>(itemInstance->takingOffIndex) >= path.size())
	{
		itemInstance->SetOrders(Orders::Order::Fly);
		return;
	}

	const AirWayPoint &wayPoint = path[itemInstance->takingOffIndex];

	float distanceFromDestinationSquared = DistanceSquared(
		itemInstance->GetX(), itemInstance->GetY(),
		airport->GetCenterX() + wayPoint.x, airport->GetCenterY() + wayPoint.y);

	if (distanceFromDestinationSquared < arrivalThreshold ||
		distanceFromDestinationSquared > itemInstance->previousDistance)
	{
		itemInstance->previousDistance = AircraftState::farthestDistance;
		itemInstance->SetDirection(static_cast<float>(wayPoint.direction));
		itemInstance->takeOffSpeed = wayPoint.speed * AircraftState::pathSpeedScale;
		itemInstance->takingOffIndex++;

		if (static_cast<size_t>(itemInstance->takingOffIndex) == path.size())
		{
			itemInstance->SetOrders(Orders::Order::Fly);
		}

		return;
	}

	itemInstance->previousDistance = distanceFromDestinationSquared;

	Advance(itemInstance, itemInstance->takeOffSpeed);
}

void Fly(AircraftState *itemInstance)
{
	itemInstance->takingOff = false;
	itemInstance->takingOffIndex = 0;
	itemInstance->landingIndex = 0;
	itemInstance->previousDistance = AircraftState::farthestDistance;
	itemInstance->minDistance = AircraftState::farthestDistance;

	if (itemInstance->GetState() == ItemStates::Attacking)
	{
		itemInstance->SetOrders(Orders::Order::Attack);
		return;
	}

	itemInstance->SetState(ItemStates::Flying);
	itemInstance->SetOrders(Orders::Order::Move);
}

void Circle(AircraftState *itemInstance)
{
	WorldState &world = WorldState::GetInstance();

	if (itemInstance->GetState() == ItemStates::Attacking && itemInstance->reloadTimeLeft <= 0.0f)
	{
		int targetIndex = LookUp::Get(itemInstance->GetTargetUid());

		if (targetIndex == -1)
		{
			NoTarget(itemInstance);
			return;
		}

		float sight = static_cast<float>(itemInstance->GetSight());

		float distanceToTargetSquared = DistanceSquared(
			itemInstance->GetX(), itemInstance->GetY(),
			world.items[targetIndex]->GetX(), world.items[targetIndex]->GetY());

		if (distanceToTargetSquared >= (sight * sight))
		{
			itemInstance->SetOrders(Orders::Order::Attack);
			return;
		}

		if (world.items[targetIndex]->GetLife() <= 0.0f)
		{
			NoTarget(itemInstance);
			return;
		}

		itemInstance->SetOrders(Orders::Order::Fire);
		return;
	}

	float distanceFromDestinationSquared = DistanceSquared(
		itemInstance->GetX(), itemInstance->GetY(),
		itemInstance->wayPointX, itemInstance->wayPointY);

	if (distanceFromDestinationSquared < ArrivalThresholdSquared(itemInstance) ||
		distanceFromDestinationSquared > itemInstance->minDistance)
	{
		itemInstance->SetSpeed(AircraftState::circlePaths[itemInstance->circleIndex].speed);

		itemInstance->SetDirection(
			WrapDirection(itemInstance->GetDirection() + 1.0f, itemInstance->GetDirections()));

		itemInstance->circleIndex++;

		if (static_cast<size_t>(itemInstance->circleIndex) == AircraftState::circlePaths.size())
		{
			itemInstance->circleIndex = 0;
		}

		itemInstance->wayPointX = AircraftState::circlePaths[itemInstance->circleIndex].x + itemInstance->GetX();
		itemInstance->wayPointY = AircraftState::circlePaths[itemInstance->circleIndex].y + itemInstance->GetY();

		itemInstance->minDistance = AircraftState::farthestDistance;
		return;
	}

	itemInstance->minDistance = distanceFromDestinationSquared;

	Advance(itemInstance, itemInstance->GetSpeed());
}

void Approach(AircraftState *itemInstance)
{
	if (itemInstance->landed)
	{
		itemInstance->SetOrders(Orders::Order::Stand);
		return;
	}

	AirportState *airport = GetAirport(itemInstance->GetOrders()->target_uid);

	if (airport == nullptr)
	{
		itemInstance->SetOrders(Orders::Order::Fly);
		return;
	}

	itemInstance->SetState(ItemStates::Flying);
	itemInstance->minDistance = AircraftState::farthestDistance;
	itemInstance->previousDistance = AircraftState::farthestDistance;
	itemInstance->landingIndex = 0;

	if (itemInstance->CanLandOnHelipad())
	{
		if (airport->helipadUid != INT_MIN && airport->helipadUid != itemInstance->GetUid())
		{
			itemInstance->SetOrders(Orders::Order::Fly);
			return;
		}

		airport->helipadUid = itemInstance->GetUid();

		itemInstance->approachPositionX = airport->GetCenterX() + airport->helipadApproachPosition.x;
		itemInstance->approachPositionY = airport->GetCenterY() + airport->helipadApproachPosition.y;

		itemInstance->takeOffPositionX = airport->GetCenterX() + airport->helipadDeployPosition.x;
		itemInstance->takeOffPositionY = airport->GetCenterY() + airport->helipadDeployPosition.y;

		itemInstance->currentHangerPosition = 0;
		itemInstance->landingDirection = airport->helipadDeployPosition.direction;
		itemInstance->SetSpeed(itemInstance->GetTopSpeed());

		itemInstance->SetOrders(Orders::Order::Approaching);
		return;
	}

	WorldState &world = WorldState::GetInstance();

	auto deployIt = world.deployMap.find(airport->GetUid());

	if (deployIt == world.deployMap.end())
	{
		itemInstance->SetOrders(Orders::Order::Fly);
		return;
	}

	auto &deploys = deployIt->second;

	for (size_t j = 0; j < deploys.size() && j < airport->hangerPositions.size(); j++)
	{
		if (std::get<2>(deploys[j]) != INT_MIN && std::get<2>(deploys[j]) != itemInstance->GetUid())
		{
			continue;
		}

		size_t runway = (j / AirportState::hangersPerRunway) % airport->approachPositions.size();

		itemInstance->approachPositionX = airport->GetCenterX() + airport->approachPositions[runway].x;
		itemInstance->approachPositionY = airport->GetCenterY() + airport->approachPositions[runway].y;

		std::get<2>(deploys[j]) = itemInstance->GetUid();

		itemInstance->currentHangerPosition = static_cast<int>(j);
		itemInstance->landingDirection = airport->hangerPositions[j].direction;

		itemInstance->takeOffPositionX = airport->GetX() + std::get<0>(deploys[j]);
		itemInstance->takeOffPositionY = airport->GetY() + std::get<1>(deploys[j]);

		itemInstance->SetSpeed(itemInstance->GetTopSpeed());

		itemInstance->SetOrders(Orders::Order::Approaching);
		return;
	}

	Log::Info("No free hanger at airport " + std::to_string(airport->GetUid()));
	itemInstance->SetOrders(Orders::Order::Fly);
}

void Approaching(AircraftState *itemInstance)
{
	itemInstance->GetOrders()->toX = itemInstance->approachPositionX;
	itemInstance->GetOrders()->toY = itemInstance->approachPositionY;

	float distanceFromDestinationSquared = DistanceSquared(
		itemInstance->GetX(), itemInstance->GetY(),
		itemInstance->approachPositionX, itemInstance->approachPositionY);

	if (distanceFromDestinationSquared < ArrivalThresholdSquared(itemInstance) ||
		distanceFromDestinationSquared > itemInstance->minDistance)
	{
		itemInstance->SetX(itemInstance->approachPositionX);
		itemInstance->SetY(itemInstance->approachPositionY);
		itemInstance->minDistance = AircraftState::farthestDistance;
		itemInstance->SetOrders(Orders::Order::Land);
		return;
	}

	itemInstance->minDistance = distanceFromDestinationSquared;

	Moving(itemInstance);
}

void FinishLanding(AircraftState *itemInstance)
{
	itemInstance->landingIndex = 0;
	itemInstance->takingOffIndex = 0;
	itemInstance->takeOffSpeed = 0.0f;
	itemInstance->takingOff = false;
	itemInstance->landed = true;
	itemInstance->SetSpeed(0.0f);
	itemInstance->SetDirection(static_cast<float>(itemInstance->landingDirection));
	itemInstance->SetState(ItemStates::Stand);
	itemInstance->SetOrders(Orders::Order::Stand);
}

void Land(AircraftState *itemInstance)
{
	itemInstance->deployUid = itemInstance->GetOrders()->target_uid;

	AirportState *airport = GetAirport(itemInstance->deployUid);

	if (airport == nullptr)
	{
		itemInstance->SetOrders(Orders::Order::Fly);
		return;
	}

	if (itemInstance->CanLandOnHelipad())
	{
		if (static_cast<size_t>(itemInstance->landingIndex) >= airport->helipadLandingPaths.size())
		{
			airport->helipadUid = itemInstance->GetUid();
			FinishLanding(itemInstance);
			return;
		}

		itemInstance->SetDirection(static_cast<float>(airport->helipadApproachPosition.direction));
		itemInstance->SetSpeed(airport->helipadApproachPosition.speed * AircraftState::pathSpeedScale);
		itemInstance->SetOrders(Orders::Order::Landing);
		return;
	}

	if (itemInstance->currentHangerPosition < 0 ||
		static_cast<size_t>(itemInstance->currentHangerPosition) >= airport->landingPaths.size())
	{
		itemInstance->SetOrders(Orders::Order::Fly);
		return;
	}

	const Vector<AirWayPoint> &path = airport->landingPaths[itemInstance->currentHangerPosition];

	if (static_cast<size_t>(itemInstance->landingIndex) >= path.size())
	{
		FinishLanding(itemInstance);
		return;
	}

	itemInstance->SetDirection(static_cast<float>(path[itemInstance->landingIndex].direction));
	itemInstance->SetSpeed(path[itemInstance->landingIndex].speed * AircraftState::pathSpeedScale);
	itemInstance->SetOrders(Orders::Order::Landing);
}

void Landing(AircraftState *itemInstance)
{
	AirportState *airport = GetAirport(itemInstance->deployUid);

	if (airport == nullptr)
	{
		itemInstance->SetOrders(Orders::Order::Fly);
		return;
	}

	const AirWayPoint *wayPoint = nullptr;

	if (itemInstance->CanLandOnHelipad())
	{
		if (static_cast<size_t>(itemInstance->landingIndex) >= airport->helipadLandingPaths.size())
		{
			itemInstance->SetOrders(Orders::Order::Land);
			return;
		}

		wayPoint = &airport->helipadLandingPaths[itemInstance->landingIndex];
	}
	else
	{
		if (itemInstance->currentHangerPosition < 0 ||
			static_cast<size_t>(itemInstance->currentHangerPosition) >= airport->landingPaths.size())
		{
			itemInstance->SetOrders(Orders::Order::Fly);
			return;
		}

		const Vector<AirWayPoint> &path = airport->landingPaths[itemInstance->currentHangerPosition];

		if (static_cast<size_t>(itemInstance->landingIndex) >= path.size())
		{
			itemInstance->SetOrders(Orders::Order::Land);
			return;
		}

		wayPoint = &path[itemInstance->landingIndex];
	}

	float distanceFromDestinationSquared = DistanceSquared(
		itemInstance->GetX(), itemInstance->GetY(),
		airport->GetCenterX() + wayPoint->x, airport->GetCenterY() + wayPoint->y);

	if (distanceFromDestinationSquared < arrivalThreshold ||
		distanceFromDestinationSquared > itemInstance->previousDistance)
	{
		itemInstance->previousDistance = AircraftState::farthestDistance;
		itemInstance->landingIndex++;
		itemInstance->SetOrders(Orders::Order::Land);
		return;
	}

	itemInstance->previousDistance = distanceFromDestinationSquared;

	Advance(itemInstance, itemInstance->GetSpeed());
}

void Attack(ItemInstance *itemInstance)
{
	auto *aircraft = static_cast<AircraftState *>(itemInstance);

	if (!itemInstance->CanAttack())
	{
		itemInstance->GetOrders()->id = -1;
		itemInstance->SetOrders(Orders::Order::Stand);
		return;
	}

	WorldState &world = WorldState::GetInstance();

	int targetUid = itemInstance->GetTargetUid();

	if (world.IsEnemyItemUnderCursor())
	{
		targetUid = world.GetItemUidThatIsUnderCursor();
	}

	int targetIndex = LookUp::Get(targetUid);

	if (targetIndex < 0 || static_cast<size_t>(targetIndex) >= world.items.size())
	{
		NoTarget(aircraft);
		return;
	}

	ItemInstance *targetInstance = world.items[targetIndex].get();

	if (targetInstance->IsAircraft() && !aircraft->CanTargetAir())
	{
		NoTarget(aircraft);
		return;
	}

	itemInstance->SetTargetUid(targetInstance->GetUid());
	itemInstance->SetState(ItemStates::Attacking);

	if (aircraft->takingOff)
	{
		itemInstance->SetOrders(Orders::Order::TakingOff);
		return;
	}

	if (aircraft->landed)
	{
		itemInstance->SetOrders(Orders::Order::TakeOff);
		return;
	}

	ReleaseHanger(aircraft);

	itemInstance->SetSpeed(aircraft->GetAttackSpeed());

	itemInstance->GetOrders()->toX = targetInstance->GetX();
	itemInstance->GetOrders()->toY = targetInstance->GetY();
	itemInstance->SetOrders(Orders::Order::MoveTo);
}

void Attacked(AircraftState *itemInstance)
{
	if (itemInstance->GetState() == ItemStates::Attacking)
	{
		return;
	}

	if (!itemInstance->CanAttack())
	{
		itemInstance->SetOrders(Orders::Order::Standing);
		return;
	}

	WorldState &world = WorldState::GetInstance();

	int attackerUid = itemInstance->GetOrders()->target_uid;
	int attackerIndex = LookUp::Get(attackerUid);

	if (attackerIndex < 0 || static_cast<size_t>(attackerIndex) >= world.items.size())
	{
		itemInstance->SetOrders(Orders::Order::Standing);
		return;
	}

	ItemInstance *attacker = world.items[attackerIndex].get();

	if (attacker == nullptr || (attacker->IsAircraft() && !itemInstance->CanTargetAir()))
	{
		itemInstance->SetOrders(Orders::Order::Standing);
		return;
	}

	itemInstance->SetTargetUid(attacker->GetUid());
	itemInstance->SetState(ItemStates::Attacking);

	itemInstance->GetOrders()->toX = attacker->GetX();
	itemInstance->GetOrders()->toY = attacker->GetY();
	itemInstance->SetOrders(Orders::Order::Attack);
}

void Firing(AircraftState *itemInstance)
{
	WorldState &world = WorldState::GetInstance();

	int targetIndex = LookUp::Get(itemInstance->GetTargetUid());

	if (targetIndex == -1)
	{
		NoTarget(itemInstance);
		return;
	}

	if (itemInstance->reloadTimeLeft > 0.0f)
	{
		return;
	}

	if (world.items[targetIndex]->GetLife() <= 0.0f)
	{
		NoTarget(itemInstance);
		return;
	}

	float sight = static_cast<float>(itemInstance->GetSight());

	float distanceToTargetSquared = DistanceSquared(
		itemInstance->GetX(), itemInstance->GetY(),
		world.items[targetIndex]->GetX(), world.items[targetIndex]->GetY());

	if (distanceToTargetSquared >= (sight * sight))
	{
		itemInstance->SetOrders(Orders::Order::Attack);
		return;
	}

	itemInstance->SetOrders(Orders::Order::Fire);
}

void Fire(AircraftState *itemInstance)
{
	WorldState &world = WorldState::GetInstance();

	String projectileName = itemInstance->GetWeapon();

	auto it = world.projectileRegistry.find(projectileName);

	if (it == world.projectileRegistry.end())
	{
		itemInstance->SetOrders(Orders::Order::Firing);
		return;
	}

	auto bullet = it->second->Clone();

	bullet->uid = itemInstance->GetUid();
	bullet->x = itemInstance->GetX();
	bullet->y = itemInstance->GetY();
	bullet->direction = itemInstance->GetDirection();

	int targetIndex = LookUp::Get(itemInstance->GetTargetUid());

	if (targetIndex != -1)
	{
		bullet->targetX = world.items[targetIndex]->GetX();
		bullet->targetY = world.items[targetIndex]->GetY();
		bullet->targetUid = world.items[targetIndex]->GetUid();
	}

	world.projectiles[projectileName].emplace_back(std::move(bullet));

	itemInstance->reloadTimeLeft = static_cast<float>(itemInstance->GetReloadTime()) / reloadFramesPerSecond;

	if (itemInstance->CanLandOnHelipad())
	{
		itemInstance->SetOrders(Orders::Order::Firing);
		return;
	}

	itemInstance->SetOrders(Orders::Order::BreakAway);
}

void BreakAway(AircraftState *itemInstance)
{
	WorldState &world = WorldState::GetInstance();

	float turn = ((itemInstance->GetUid() % 2) == 0) ? -1.0f : 1.0f;

	itemInstance->SetDirection(
		WrapDirection(itemInstance->GetDirection() + turn, itemInstance->GetDirections()));

	float movement = itemInstance->GetSpeed() * movementScale * world.GetDeltaTime();

	size_t index = DirectionTableIndex(itemInstance);

	itemInstance->breakAwayToX =
		(movement * AircraftState::sinDirectionAngles[index] * breakAwayDistance) + itemInstance->GetX();
	itemInstance->breakAwayToY =
		(-movement * AircraftState::cosDirectionAngles[index] * breakAwayDistance) + itemInstance->GetY();

	itemInstance->minDistance = AircraftState::farthestDistance;

	itemInstance->SetOrders(Orders::Order::BreakingAway);
}

void BreakingAway(AircraftState *itemInstance)
{
	float distanceFromDestinationSquared = DistanceSquared(
		itemInstance->GetX(), itemInstance->GetY(),
		itemInstance->breakAwayToX, itemInstance->breakAwayToY);

	if (distanceFromDestinationSquared < ArrivalThresholdSquared(itemInstance) ||
		distanceFromDestinationSquared > itemInstance->minDistance)
	{
		EnterCircle(itemInstance);
		itemInstance->SetOrders(Orders::Order::Circle);
		return;
	}

	itemInstance->minDistance = distanceFromDestinationSquared;

	Advance(itemInstance, itemInstance->GetSpeed());
}

void NoTarget(AircraftState *itemInstance)
{
	itemInstance->SetTargetUid(0);
	itemInstance->SetState(ItemStates::Flying);

	if (itemInstance->landed)
	{
		itemInstance->SetState(ItemStates::Stand);
		itemInstance->SetOrders(Orders::Order::Stand);
		return;
	}

	EnterCircle(itemInstance);
	itemInstance->SetOrders(Orders::Order::Circle);
}

void Destroyed(ItemInstance *itemInstance)
{
	WorldState &world = WorldState::GetInstance();

	ReleaseHanger(static_cast<AircraftState *>(itemInstance));

	String removeItem = StringConcat("uid:", itemInstance->GetUid());
	world.gameEvents.emplace_back(UIAction::RemoveGameItem, removeItem);
}

void Animate(AircraftState *itemInstance)
{
	auto &aircraft = aircrafts[itemInstance->GetUid()];

	if (itemInstance->GetFrames() < itemInstance->GetDirections() * 2)
	{
		aircraft->animationOffset = 0;
		return;
	}

	if (itemInstance->landed && itemInstance->GetSpeed() < itemInstance->GetGroundSpeed())
	{
		aircraft->animationOffset = 0;
		return;
	}

	if ((aircraft->animationSpeed % itemInstance->animationSpeedLimit) == 0)
	{
		if (aircraft->animationCount < itemInstance->animationLimit)
		{
			aircraft->animationCount++;
		}
		else
		{
			aircraft->animationCount = 0;

			if (aircraft->animationOffset == 0)
			{
				aircraft->animationOffset = itemInstance->GetTakeOffLandingOffset();
			}
			else
			{
				aircraft->animationOffset = 0;
			}
		}
	}

	aircraft->animationSpeed++;
}

void RegisterToQuadTree(const Set<String> &groups)
{
	Physics &physics = Physics::GetInstance();
	physics.Add(groups);
	physics.Show();
}
} // namespace TGX
