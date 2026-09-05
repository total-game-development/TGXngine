#include "Ships.h"

#include <algorithm>
#include "Collision/Collision.h"
#include "DeployBerths.h"
#include "Enums.h"
#include "ImageLoader.h"
#include "Logs.h"
#include "Lookup.h"
#include "Navigation.h"
#include "Orders.h"
#include "Physics.h"
#include "StringUtils.hpp"
#include "module_interface.h"

#include <iostream>

namespace TGX
{
extern "C"
{
	MODULE_API void OutputTest()
	{
	}

	MODULE_API void Init()
	{
		Navigation::Init();

		if (orderMap.empty())
		{
			orderMap = {
				{Orders::Order::Action, [](ItemInstance *state) {
					 Action((state));
				 }},
				{Orders::Order::Move, [](ItemInstance *state) {
					 Move((state));
				 }},
				{Orders::Order::MoveTo, [](ItemInstance *state) {
					 MoveTo((state));
				 }},
				{Orders::Order::Turning, [](ItemInstance *state) {
					 Turning(static_cast<ShipState *>(state));
				 }},
				{Orders::Order::Moving, [](ItemInstance *state) {
					 Moving(static_cast<ShipState *>(state));
				 }},
				{Orders::Order::Standing, [](ItemInstance *state) {
					 Standing(static_cast<ShipState *>(state));
				 }},
				{Orders::Order::Stand, [](ItemInstance * /*state*/) {
					 Stand();
				 }},
				{Orders::Order::Attack, [](ItemInstance *state) {
					 Attack(state);
				 }},
				{Orders::Order::Attacked, [](ItemInstance *state) {
					 Attacked(static_cast<ShipState *>(state));
				 }},
				{Orders::Order::TurnToFire, [](ItemInstance *state) {
					 TurnToFire(static_cast<ShipState *>(state));
				 }},
				{Orders::Order::Firing, [](ItemInstance *state) {
					 Firing(static_cast<ShipState *>(state));
				 }},
				{Orders::Order::Fire, [](ItemInstance *state) {
					 Fire(static_cast<ShipState *>(state));
				 }},
				{Orders::Order::Extract, [](ItemInstance *state) {
					 Extract(state);
				 }},
				{Orders::Order::Destroyed, [](ItemInstance *state) {
					 Destroyed(state);
				 }},
				{Orders::Order::Unload, [](ItemInstance *state) {
					 Unload(static_cast<ShipState *>(state));
				 }},
			};
		}

		Log::Success("ShipState Init created");
	}

	MODULE_API ItemInstance *Awake(const String &name)
	{
		Log::Success("ShipState Awake " + name);

		if (name == "submarine")
		{
			return globalItem = new SubmarineState();
		}
		if (name == "battleship")
		{
			return globalItem = new BattleshipState();
		}
		if (name == "cruiser")
		{
			return globalItem = new CruiserState();
		}
		if (name == "carrier")
		{
			return globalItem = new CarrierState();
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

		ships[globalItem->GetUid()] = std::make_unique<Ships>(globalItem);

		WorldState &world = WorldState::GetInstance();

		if (globalItem->IsTraining())
		{
			int index = LookUp::Get(world.GetPrimaryItem(globalItem->GetTeam(), "shipyard"));

			if (index != -1)
			{
				int deployUid = world.items[index]->GetUid();
				auto deployIt = world.deployMap.find(deployUid);

				if (deployIt != world.deployMap.end())
				{
					auto &deploys = deployIt->second;

					DeployBerths::ReleaseVacated(world.items[index].get(), deploys);

					size_t i = 0;

					for (; i < deploys.size(); i++)
					{
						if (std::get<2>(deploys[i]) == INT_MIN)
						{
							std::get<2>(deploys[i]) = globalItem->GetUid();
							break;
						}
					}

					// Every berth taken still has to put the unit somewhere. At the
					// last berth it is at least at the building, where steering can push
					// it clear; with no position at all it appears in the map corner.
					const size_t berth = (i < deploys.size()) ? i : deploys.size() - 1;

					globalItem->SetX(world.items[index]->GetX() + std::get<0>(deploys[berth]));
					globalItem->SetY(world.items[index]->GetY() + std::get<1>(deploys[berth]));
					globalItem->SetDirection(6);
				}
			}
			else
			{
				Log::Error("Barracks item not found!");
			}
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

		Physics &physics = Physics::GetInstance();
		globalItem->AddToGrid(world.currentTerrainMapPassableGrid, physics.GetGridTracker());

		RegisterToQuadTree(globalItem->GetGroups());
		Log::Success("Ship " + name + " created successfully");
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
		ships[itemInstance->GetUid()]->Draw(itemInstance, spritesRef);
	}

	MODULE_API void Update(ItemInstance *itemInstance, std::vector<sf::Sprite *> *spritesRef)
	{
		if (itemInstance->GetHidden())
		{
			return;
		}

		WorldState &world = WorldState::GetInstance();

		const bool isVisible = itemInstance->GetTeam() == WorldState::GetInstance().GetTeam() || itemInstance->isVisible();

		if (HoverOverHull(itemInstance) && isVisible)
		{
			world.SetItemUidThatIsUnderCursor(itemInstance->GetUid());

			if (world.GetTeam() == itemInstance->GetTeam())
			{
				world.SetItemUnderCursor(true);

				if (world.IsLeftClicked())
				{
					world.selected.emplace_back(itemInstance->GetUid());
					Log::Success("Ship added to selection list (size: " + std::to_string(world.selected.size()) + ")");
				}
			}
			else
			{
				world.SetLoadableItemUnderCursor(false);

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

								Log::Info("Enemy Infantry has been right clicked");
								Log::Info("Enemy Uid: " + std::to_string(itemInstance->GetUid()));
								Log::Info("Enemy GetTeam: " + itemInstance->GetTeam());
							}
						}
					}
				}
			}
		}

		if (world.selected.size() != 0 && itemInstance->CanLoad())
		{
			auto *thresholdPtr = itemInstance->GetExtra<int>(ItemInstance::ItemProperty::LoadThreshold);
			auto *loadedUIDs = itemInstance->GetExtra<std::vector<int>>(ItemInstance::ItemProperty::LoadedUIDs);
			float loadThreshold = static_cast<float>(thresholdPtr ? *thresholdPtr : 0);
			int currentLoadCount = loadedUIDs ? static_cast<int>(loadedUIDs->size()) : 0;

			if (currentLoadCount > 0)
			{
				if (HoverFromCenter(itemInstance, loadThreshold))
				{
					world.SetLoadableItemUnderCursor(true);
				}
				else
				{
					world.SetLoadableItemUnderCursor(false);
				}
			}
			else
			{
				world.SetLoadableItemUnderCursor(false);
			}
		}

		int frame = itemInstance->GetFrame();

		if (spritesRef == nullptr || frame < 0 || frame >= static_cast<int>(spritesRef->size()))
		{
			return;
		}

		(*spritesRef)[frame]->setPosition(
			sf::Vector2f(
				((itemInstance->GetX() * Globals::grid_size) + static_cast<float>(world.GetMapXOffset())),
				((itemInstance->GetY() * Globals::grid_size) + static_cast<float>(world.GetMapYOffset()))));

		itemInstance->SetFrame(
			(static_cast<int>(round(itemInstance->GetDirection())) % itemInstance->GetDirections()) + (ships[itemInstance->GetUid()]->animationOffset));

		ships[itemInstance->GetUid()]->Update(itemInstance, spritesRef);
	}

	MODULE_API void Delete(int uid)
	{
		auto it = ships.find(uid);
		if (it != ships.end())
		{
			ships.erase(it);
		}

		Log::Clean("Ship deleted. Remaining ships: " + std::to_string(ships.size()));
	}

	MODULE_API void Destroy(const std::string &name)
	{
		Log::Clean("Destroy static ship { " + name + " }");
	}
}

void Action(ItemInstance *itemInstance)
{
	WorldState &world = WorldState::GetInstance();

	Orders::Order order = Orders::Order::Stand;

	if (world.IsEnemyItemUnderCursor())
	{
		order = Orders::Order::Attack;
	}
	else if (world.IsResourceUnderCursor())
	{
		order = Orders::Order::Extract;
	}
	else if (world.IsLoadableItemUnderCursor())
	{
		order = Orders::Order::Unload;
	}

	itemInstance->SetOrders(order);
}

void ReleaseDeployBerth(int uid)
{
	WorldState &world = WorldState::GetInstance();

	for (auto &[buildingUid, deploys] : world.deployMap)
	{
		for (auto &deploy : deploys)
		{
			if (std::get<2>(deploy) == uid)
			{
				std::get<2>(deploy) = INT_MIN;
				Log::Info("Yard berth released by UID " + std::to_string(uid));
				return;
			}
		}
	}
}

void Move(ItemInstance *itemInstance)
{
	Log::Print("Move : " + std::to_string(itemInstance->GetUid()));

	ReleaseDeployBerth(itemInstance->GetUid());

	WorldState &world = WorldState::GetInstance();
	Physics &physics = Physics::GetInstance();

	itemInstance->RemoveFromGrid(world.currentTerrainMapPassableGrid, physics.GetGridTracker());
	static_cast<ShipState *>(itemInstance)->RemoveTacticalGrid(itemInstance->GetUid(), world.currentTerrainMapPassableGrid, physics.GetGridTracker());

	itemInstance->SetOrders(Orders::Order::MoveTo);
}

void MoveTo(ItemInstance *itemInstance)
{
	Log::Print("MoveTo - UID: " + std::to_string(itemInstance->GetUid()));

	WorldState &world = WorldState::GetInstance();
	Physics &physics = Physics::GetInstance();

	auto *shipState = static_cast<ShipState *>(itemInstance);

	if (itemInstance->GetState() == ItemStates::Attacking)
	{
		int targetIndex = LookUp::Get(itemInstance->GetTargetUid());

		if (targetIndex != -1)
		{
			const ItemInstance *target = world.items[targetIndex].get();
			const float sight = static_cast<float>(shipState->GetSight());

			float rangeX = target->GetX() - itemInstance->GetX();
			float rangeY = target->GetY() - itemInstance->GetY();

			if (((rangeX * rangeX) + (rangeY * rangeY)) < (sight * sight))
			{
				shipState->hasNextStep = false;
				itemInstance->SetOrders(Orders::Order::Standing);
				return;
			}

			world.items[targetIndex]->RemoveFromGrid(
				world.currentTerrainMapPassableGrid,
				physics.GetGridTracker());

			SetPath(shipState, world.items[targetIndex]->GetX(), world.items[targetIndex]->GetY());

			Log::Debug("(Re)AddToGrid for Target");
			world.items[targetIndex]->AddToGrid(
				world.currentTerrainMapPassableGrid,
				physics.GetGridTracker());
		}
	}
	else
	{
		SetPath(shipState, itemInstance->GetOrders()->toX, itemInstance->GetOrders()->toY);
	}

	if (!shipState->path.empty())
	{
		shipState->hasNextStep = false;

		if (itemInstance->GetState() == ItemStates::Attacking)
		{
			int targetIndex = LookUp::Get(itemInstance->GetTargetUid());

			if (targetIndex != -1)
			{
				SetTacticalCoordinates(
					itemInstance,
					shipState->path,
					world.items[targetIndex]->GetCenterX(),
					world.items[targetIndex]->GetCenterY(),
					shipState->GetSight() +
						static_cast<int>(world.items[targetIndex]->GetOuterSight()));
			}
		}

		itemInstance->SetOrders(Orders::Order::Turning);
		Log::Success("Path found for UID " + std::to_string(itemInstance->GetUid()));
	}
	else
	{
		itemInstance->SetOrders(Orders::Order::Standing);
		itemInstance->SetState(ItemStates::Stand);
		Log::Error("No path found for UID " + std::to_string(itemInstance->GetUid()) + " - entering standing state.");
	}
}

void Turning(ShipState *itemInstance)
{
	Log::Info("Turning - UID: " + std::to_string(itemInstance->GetUid()));

	if (itemInstance->GetState() == ItemStates::Attacking)
	{
		itemInstance->SetOrders(Orders::Order::Moving);
		Log::Info("Ship transitioning to attacking order.");
	}
	else
	{
		itemInstance->SetSpeed(itemInstance->GetSpeed());
		itemInstance->SetOrders(Orders::Order::Moving);
		Log::Success("Ship transitioning to moving order.");
	}
}

void Moving(ShipState *itemInstance)
{
	WorldState &world = WorldState::GetInstance();
	auto &path = itemInstance->path;

	if (!itemInstance->hasNextStep)
	{
		if (path.empty())
		{
			Log::Error("Error: Path is empty. Cannot initialize next step. UID: " + std::to_string(itemInstance->GetUid()));
			itemInstance->SetOrders(Orders::Order::Standing);
			return;
		}

		const auto &nextPathStep = path.back();
		itemInstance->nextStep.x = nextPathStep.x;
		itemInstance->nextStep.y = nextPathStep.y;
		itemInstance->hasNextStep = true;
		Log::Info("Initialized next step for UID " + std::to_string(itemInstance->GetUid()));
	}

	float dx = itemInstance->GetX() - static_cast<float>(itemInstance->nextStep.x);
	float dy = itemInstance->GetY() - static_cast<float>(itemInstance->nextStep.y);
	float distanceFromDestinationSquared = (dx * dx) + (dy * dy);

	if (itemInstance->GetState() == ItemStates::Attacking)
	{
		int targetIndex = LookUp::Get(itemInstance->GetTargetUid());

		if (targetIndex != -1)
		{
			const float stopAt = static_cast<float>(itemInstance->GetSight()) +
								 world.items[targetIndex]->GetOuterSight();

			if (std::pow(world.items[targetIndex]->GetCenterX() - itemInstance->GetX(), 2) +
					std::pow(world.items[targetIndex]->GetCenterY() - itemInstance->GetY(), 2) <
				(stopAt * stopAt))
			{
				itemInstance->hasNextStep = false;
				itemInstance->SetOrders(Orders::Order::Standing);
				return;
			}
		}
	}

	if (distanceFromDestinationSquared < 0.01)
	{
		if (!path.empty())
		{
			path.pop_back();

			if (!path.empty())
			{
				const auto &nextPathStep = path.back();
				itemInstance->nextStep.x = nextPathStep.x;
				itemInstance->nextStep.y = nextPathStep.y;
				Log::Info("Advancing to next path step for UID " + std::to_string(itemInstance->GetUid()));
			}
			else
			{
				if (itemInstance->GetState() == ItemStates::Extracting)
				{
					Log::Success("Path complete for UID " + std::to_string(itemInstance->GetUid()) + " switch to build a extractor.");

					for (size_t i = 0; i < world.resources.size(); i++)
					{
						if (world.resources[i]->GetUid() == itemInstance->GetTargetUid())
						{
							String builtCommand = StringConcat("command:", "build");
							builtCommand += ",";
							String builtName = StringConcat("name:", world.resources[i]->GetName() + "_extractor");
							builtCommand += builtName + ",";
							String builtType = StringConcat("type:", "buildings");
							builtCommand += builtType + ",";
							String builtTeam = StringConcat("team:", world.GetTeam());
							builtCommand += builtTeam + ",";
							String builtX = StringConcat("x:", world.resources[i]->GetX());
							builtCommand += builtX + ",";
							String builtY = StringConcat("y:", world.resources[i]->GetY());
							builtCommand += builtY;

							world.extractors[world.GetTeam()][world.resources[i]->GetName()]++;
							world.gameEvents.emplace_back(UIAction::AddGameItem, builtCommand);
							itemInstance->SetOrders(Orders::Order::Destroyed);
							return;
						}
					}
				}
				else
				{
					Log::Success("Path complete for UID " + std::to_string(itemInstance->GetUid()) + " switching to standing order.");
					itemInstance->hasNextStep = false;
					itemInstance->SetOrders(Orders::Order::Standing);
					return;
				}
			}
		}
	}

	float newDirection = FindAngle(
		static_cast<float>(itemInstance->nextStep.x), static_cast<float>(itemInstance->nextStep.y),
		itemInstance->GetX(), itemInstance->GetY(),
		itemInstance->GetDirections());

	float difference = AngleDiff(
		itemInstance->GetDirection(),
		newDirection,
		itemInstance->GetDirections());

	float movement = 0.0;
	float turnAmount = static_cast<float>(itemInstance->GetTurnSpeed()) * (1.0f / 8.0f) * world.GetDeltaTime();

	if (std::abs(difference) > turnAmount)
	{
		float direction = itemInstance->GetDirection() + (turnAmount * std::abs(difference) / difference);
		itemInstance->SetDirection(WrapDirection(direction, itemInstance->GetDirections()));
	}
	else
	{
		itemInstance->SetDirection(
			WrapDirection(itemInstance->GetDirection() + difference, itemInstance->GetDirections()));

		movement =
			itemInstance->GetSpeed() *
			ShipState::accelerationFactor[itemInstance->accelerationIndex] *
			(1.0f / 96.0f) * world.GetDeltaTime();

		float angleRadians =
			-(itemInstance->GetDirection() / static_cast<float>(itemInstance->GetDirections()) * 2.0f * PI);

		float moveX = -(movement * std::sin(angleRadians));
		float moveY = -(movement * std::cos(angleRadians));

		itemInstance->SetLastMovementX(moveX);
		itemInstance->SetLastMovementY(moveY);
		itemInstance->SetX(itemInstance->GetX() + moveX);
		itemInstance->SetY(itemInstance->GetY() + moveY);
	}

	Steering(itemInstance);
}

void Animate(ShipState *itemState)
{
	if (itemState->GetFrames() < itemState->GetDirections() * 2)
	{
		ships[itemState->GetUid()]->animationOffset = 0;
		return;
	}

	if ((ships[itemState->GetUid()]->animationSpeed % itemState->animationSpeedLimit) == 0)
	{
		if (ships[itemState->GetUid()]->animationCount < itemState->animationLimit)
		{
			ships[itemState->GetUid()]->animationCount++;
		}
		else
		{
			if (ships[itemState->GetUid()]->animationOffset == 0)
			{
				ships[itemState->GetUid()]->animationOffset = 8;
			}
			else
			{
				ships[itemState->GetUid()]->animationOffset = 0;
			}
		}
	}

	ships[itemState->GetUid()]->animationSpeed++;
}

void Standing(ShipState *itemInstance)
{
	WorldState &world = WorldState::GetInstance();
	Physics &physics = Physics::GetInstance();

	itemInstance->AddToGrid(world.currentTerrainMapPassableGrid, physics.GetGridTracker());
	itemInstance->RemoveTacticalGrid(itemInstance->GetUid(), world.currentTerrainMapPassableGrid, physics.GetGridTracker());

	if (itemInstance->GetState() == ItemStates::Attacking)
	{
		itemInstance->SetOrders(Orders::Order::TurnToFire);
	}
	else
	{
		itemInstance->SetOrders(Orders::Order::Stand);
	}
}

void Stand()
{
}

void SetPath(ShipState *itemInstance, float toX, float toY)
{
	WorldState &world = WorldState::GetInstance();

	itemInstance->path = Navigation::GetInstance().GetPath(
		world.currentIsleMapPassableGrid,
		{itemInstance->GetX(), itemInstance->GetY()},
		{toX, toY},
		itemInstance->GetCellCollisionMode(),
		PathfindingAlgorithm::Naval);
}

void OnPath(const Vector<Point> &path)
{
	Log::Info("onPath called with path size: " + std::to_string(path.size()));
}

void SetTacticalCoordinates(ItemInstance *itemInstance, Vector<Point> path, float toX, float toY, int sight)
{
	size_t pathIndex = 0;

	// The booked cell is where the unit means to stop, so it has to be the
	// same distance the range test uses -- the caller folds the target's
	// outer sight into what it passes.

	while (pow(toX - static_cast<float>(path[pathIndex].x), 2) + pow(toY - static_cast<float>(path[pathIndex].y), 2) < pow(sight, 2))
	{
		pathIndex++;

		if (pathIndex == path.size())
		{
			return;
		}
	}

	WorldState &world = WorldState::GetInstance();
	Physics &physics = Physics::GetInstance();

	static_cast<ShipState *>(itemInstance)->AddTacticalGrid(itemInstance->GetUid(), static_cast<int>(pathIndex), path, static_cast<int>(itemInstance->GetRadius()) / 20, world.currentTerrainMapPassableGrid, physics.GetGridTracker());

	std::ranges::reverse(path);
}

void Steering(ItemInstance *itemInstance)
{
	float searchRadius = 5.0;
	Boundary nearbyRange(
		itemInstance->GetX() - searchRadius,
		itemInstance->GetY() - searchRadius,
		itemInstance->GetX() + searchRadius,
		itemInstance->GetY() + searchRadius);

	Physics &physics = Physics::GetInstance();

	Set<String> layerGroup{static_cast<ShipState *>(itemInstance)->GetLayerGroup()};

	Vector<PointUID> foundUnits = physics.Find(layerGroup, nearbyRange);

	if (!foundUnits.empty())
	{
		Vector<ItemInstance *> nearbyItems;

		WorldState &world = WorldState::GetInstance();
		for (auto &foundUnit : foundUnits)
		{
			if (itemInstance->GetUid() == foundUnit.uid)
			{
				continue;
			}

			int index = LookUp::Get(foundUnit.uid);
			if (index != -1 && static_cast<size_t>(index) < world.items.size())
			{
				nearbyItems.emplace_back(world.items[index].get());
			}
		}

		Vector<ItemInstance *> collidedBodyItems = Detect(itemInstance, nearbyItems);

		for (auto &collidedItem : collidedBodyItems)
		{
			if (itemInstance->GetOrders()->order == Orders::Order::Moving &&
				collidedItem->GetOrders()->order == Orders::Order::Moving)
			{
				if (FindFurthestItem(
						itemInstance,
						collidedItem,
						static_cast<ShipState *>(itemInstance)->nextStep,
						static_cast<ShipState *>(collidedItem)->nextStep))
				{
					Stop(itemInstance);
				}
			}

			// Both have to actually be in the same commanded group. An order id of 0
			// is what a unit has when nobody has grouped it -- every AI unit, and
			// anything freshly built -- so matching on it stood down any unit that
			// so much as brushed another one parked at the factory.
			if ((itemInstance->GetOrders()->id > 0) &&
				(itemInstance->GetOrders()->id == collidedItem->GetOrders()->id) &&
				(collidedItem->GetOrders()->order == Orders::Order::Stand ||
				 collidedItem->GetOrders()->order == Orders::Order::Standing))
			{
				Log::Info("Collision with standing unit, UID: " + std::to_string(itemInstance->GetUid()));

				itemInstance->SetOrders(Orders::Order::Standing);
				return;
			}
		}
	}

	Velocity(itemInstance);
}

Vector<ItemInstance *> Detect(ItemInstance *itemInstance, const Vector<ItemInstance *> &nearByItems)
{
	Vector<ItemInstance *> collidedItems;

	for (const auto &collisionItem : nearByItems)
	{
		// Ships against ships only. The client queries a ships-only tree here
		// rather than the army one, so nothing on land is ever a candidate.
		if (!collisionItem || collisionItem == itemInstance || collisionItem->GetType() != "ships")
		{
			continue;
		}

		if (SATCollision(itemInstance->polygon, collisionItem->polygon) > 0.0f)
		{
			collidedItems.emplace_back(collisionItem);
		}
	}

	return collidedItems;
}

void RegisterToQuadTree(const Set<String> &groups)
{
	Physics &physics = Physics::GetInstance();
	physics.Add(groups);
	physics.Show();
}

void Stop(ItemInstance *item)
{
	// The slowest factor is the last one in the table, not one past it. Reading
	// off the end is what tripped the debug assertion, and in a release build it
	// scaled the speed by whatever happened to sit after the array.
	static_cast<ShipState *>(item)->accelerationIndex = ShipState::accelerationFactor.size() - 1;
};

void Velocity(ItemInstance *itemInstance)
{
	if (static_cast<int>(static_cast<ShipState *>(itemInstance)->accelerationIndex) == static_cast<ShipState *>(itemInstance)->velocityThreshold)
	{
		return;
	}

	if (static_cast<int>(static_cast<ShipState *>(itemInstance)->accelerationIndex) < static_cast<ShipState *>(itemInstance)->velocityThreshold)
	{
		static_cast<ShipState *>(itemInstance)->accelerationIndex++;
	}

	if (static_cast<int>(static_cast<ShipState *>(itemInstance)->accelerationIndex) > static_cast<ShipState *>(itemInstance)->velocityThreshold)
	{
		static_cast<ShipState *>(itemInstance)->accelerationIndex--;
	}
}

void Attack(ItemInstance *itemInstance)
{
	Log::Debug("Attack");

	ReleaseDeployBerth(itemInstance->GetUid());

	if (!itemInstance->CanAttack())
	{
		Log::Debug("Can't attack");
		itemInstance->GetOrders()->id = -1;
		itemInstance->SetOrders(Orders::Order::Stand);
		return;
	}

	WorldState &world = WorldState::GetInstance();
	Physics &physics = Physics::GetInstance();

	int targetIndex = LookUp::Get(world.GetItemUidThatIsUnderCursor());
	Log::Info("Attack - UID: " + std::to_string(itemInstance->GetUid()) + " Target Index: " + std::to_string(targetIndex));

	if (targetIndex < 0 || static_cast<size_t>(targetIndex) >= world.items.size())
	{
		itemInstance->SetTargetUid(0);
		return;
	}

	itemInstance->RemoveFromGrid(
		world.currentTerrainMapPassableGrid,
		physics.GetGridTracker());

	ItemInstance *targetInstance = world.items[targetIndex].get();
	itemInstance->SetState(ItemStates::Attacking);

	itemInstance->SetTargetUid(targetInstance->GetUid());

	itemInstance->GetOrders()->toX = targetInstance->GetX();
	itemInstance->GetOrders()->toY = targetInstance->GetY();
	itemInstance->SetOrders(Orders::Order::MoveTo);
}

void Attacked(ShipState *itemInstance)
{
	if (itemInstance == nullptr)
	{
		Log::Warning("Attacked() called with null item instance.");
		return;
	}

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
	Physics &physics = Physics::GetInstance();

	int attackerUid = itemInstance->GetOrders()->target_uid;
	int attackerIndex = LookUp::Get(attackerUid);

	if (attackerIndex < 0 || attackerIndex >= static_cast<int>(world.items.size()))
	{
		Log::Warning("Attacked: no such attacker for UID: " + std::to_string(attackerUid));
		itemInstance->SetOrders(Orders::Order::Standing);
		return;
	}

	ItemInstance *attacker = world.items[attackerIndex].get();

	if (attacker == nullptr)
	{
		Log::Warning("Attacked: attacker is null for UID: " + std::to_string(attackerUid));
		itemInstance->SetOrders(Orders::Order::Standing);
		return;
	}

	itemInstance->RemoveFromGrid(
		world.currentTerrainMapPassableGrid,
		physics.GetGridTracker());

	itemInstance->SetState(ItemStates::Attacking);

	itemInstance->SetTargetUid(attacker->GetUid());

	itemInstance->GetOrders()->toX = attacker->GetX();
	itemInstance->GetOrders()->toY = attacker->GetY();
	itemInstance->SetOrders(Orders::Order::MoveTo);

	Log::Info("Returning fire - UID: " + std::to_string(itemInstance->GetUid()) +
			  " attacker: " + std::to_string(attacker->GetUid()));
}

void TurnToFire(ShipState *itemInstance)
{
	itemInstance->SetState(ItemStates::Firing);
	itemInstance->SetOrders(Orders::Order::Firing);
}

constexpr float reloadFramesPerSecond = 60.0f;

void Firing(ShipState *itemInstance)
{
	WorldState &world = WorldState::GetInstance();

	int targetIndex = LookUp::Get(itemInstance->GetTargetUid());

	if (targetIndex == -1)
	{
		Log::Error("Target State is nullptr");
		itemInstance->SetOrders(Orders::Order::Standing);
		return;
	}

	if (itemInstance->reloadTimeLeft > 0.0f)
	{
		itemInstance->reloadTimeLeft -= world.GetDeltaTime();
		return;
	}

	if (world.items[targetIndex]->GetLife() <= 0)
	{
		itemInstance->reloadTimeLeft = 0.0f;
		itemInstance->SetOrders(Orders::Order::Standing);
		return;
	}

	float dx = world.items[targetIndex]->GetCenterX() - itemInstance->GetX();
	float dy = world.items[targetIndex]->GetCenterY() - itemInstance->GetY();

	// Sight plus the target's own reach, the way getTargetOuterSight does it in
	// the client. Without the second term a unit at the wall of anything bigger
	// than its sight is forever out of range and stands there.
	const float range = static_cast<float>(itemInstance->GetSight()) +
						world.items[targetIndex]->GetOuterSight();

	if (((dx * dx) + (dy * dy)) < (range * range))
	{
		itemInstance->reloadTimeLeft = static_cast<float>(itemInstance->GetReloadTime()) / reloadFramesPerSecond;
		itemInstance->SetOrders(Orders::Order::Fire);
	}
	else
	{
		itemInstance->SetOrders(Orders::Order::Standing);
	}
}

void Fire(ShipState *itemInstance)
{
	Log::Info("Fire");

	WorldState &world = WorldState::GetInstance();

	String projectileName = itemInstance->GetWeapon();

	auto it = world.projectileRegistry.find(projectileName);
	if (it == world.projectileRegistry.end())
	{
		Log::Warning("Fire: no projectile registered for weapon '" + projectileName + "'");
		itemInstance->SetOrders(Orders::Order::Standing);

		return;
	}

	Log::Debug("Bullet");

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

	itemInstance->SetOrders(Orders::Order::Firing);
}

void Extract(ItemInstance *itemInstance)
{
	Log::Debug("Extract");

	if (!itemInstance->CanExtract())
	{
		Log::Debug("Can't Extract");
		itemInstance->GetOrders()->id = -1;
		itemInstance->SetOrders(Orders::Order::Stand);
		return;
	}

	WorldState &world = WorldState::GetInstance();
	Physics &physics = Physics::GetInstance();

	Log::Info(world.GetResourceUidThatIsUnderCursor());

	for (size_t i = 0; i < world.resources.size(); i++)
	{
		if (world.resources[i]->GetUid() == world.GetResourceUidThatIsUnderCursor())
		{
			itemInstance->RemoveFromGrid(
				world.currentTerrainMapPassableGrid,
				physics.GetGridTracker());

			ResourceInstance *targetInstance = world.resources[i].get();
			itemInstance->SetState(ItemStates::Extracting);

			itemInstance->SetTargetUid(targetInstance->GetUid());

			itemInstance->GetOrders()->toX = targetInstance->GetX();
			itemInstance->GetOrders()->toY = targetInstance->GetY();
			itemInstance->SetOrders(Orders::Order::MoveTo);
			break;
		}
	}
}

void Destroyed(ItemInstance *itemInstance)
{
	WorldState &world = WorldState::GetInstance();
	Physics &physics = Physics::GetInstance();

	String removeItem = StringConcat("uid:", itemInstance->GetUid());
	world.gameEvents.emplace_back(UIAction::RemoveGameItem, removeItem);

	itemInstance->RemoveFromGrid(
		world.currentTerrainMapPassableGrid,
		physics.GetGridTracker());

	static_cast<ShipState *>(itemInstance)->RemoveTacticalGrid(
		itemInstance->GetUid(),
		world.currentTerrainMapPassableGrid,
		physics.GetGridTracker());
}

void Unload(ItemInstance *itemInstance)
{
	Log::Info("Unload");

	auto *loadedUIDs = itemInstance->GetExtra<Vector<int>>(ItemInstance::ItemProperty::LoadedUIDs);

	int unitUid = loadedUIDs->back();
	loadedUIDs->pop_back();

	WorldState &world = WorldState::GetInstance();
	int unitIndex = LookUp::Get(unitUid);

	if (unitIndex != -1)
	{
		ItemInstance *unloadedItem = world.items[unitIndex].get();

		unloadedItem->SetX((world.GetGameX()) / Globals::grid_size);
		unloadedItem->SetY((world.GetGameY() - 80.0f) / Globals::grid_size);

		unloadedItem->SetOrders(Orders::Order::Standing);
		unloadedItem->SetHidden(false);

		Log::Success("Unit " + std::to_string(unitUid) + " successfully dropped at game coordinates.");
	}

	itemInstance->SetOrders(Orders::Order::Stand);
}

void TestPhysics()
{
	Log::Print("Test Physics");
}

void TestSearch()
{
	std::vector<std::vector<int>> grid;

	size_t height = 500;
	size_t width = 500;

	grid.resize(height);

	for (size_t i = 0; i < height; i++)
	{
		grid[i].resize(width);
	}

	for (size_t y = 0; y < height; y++)
	{
		for (size_t x = 0; x < width; x++)
		{
			grid[y][x] = 0;
		}
	}
}
} // namespace TGX
