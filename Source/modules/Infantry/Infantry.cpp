#include "Infantry.h"
#include "Collision/Collision.h"
#include "DeployBerths.h"
#include "Enums.h"
#include "ImageLoader.h"
#include "Logs.h"
#include "Lookup.h"
#include "Navigation.h"
#include "NextStep.h"
#include "Orders.h"
#include "PathFinding/IPathfinding.h"
#include "Physics.h"
#include "Physics/Vector2D.h"
#include "Point.h"
#include "ProjectileInstance.h"
#include "StringUtils.hpp"
#include "module_interface.h"

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
				{Orders::Order::Attack, [](ItemInstance *state) {
					 Attack((state));
				 }},
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
					 Turning(static_cast<InfantryState *>(state));
				 }},
				{Orders::Order::TurnToFire, [](ItemInstance *state) {
					 TurnToFire(static_cast<InfantryState *>(state));
				 }},
				{Orders::Order::Firing, [](ItemInstance *state) {
					 Firing(static_cast<InfantryState *>(state));
				 }},
				{Orders::Order::Fire, [](ItemInstance *state) {
					 Fire(static_cast<InfantryState *>(state));
				 }},
				{Orders::Order::Moving, [](ItemInstance *state) {
					 Moving(static_cast<InfantryState *>(state));
				 }},
				{Orders::Order::Attacked, [](ItemInstance *state) {
					 Attacked(static_cast<InfantryState *>(state));
				 }},
				{Orders::Order::Search, [](ItemInstance *state) {
					 Search(static_cast<InfantryState *>(state));
				 }},
				{Orders::Order::SearchAndDestroy, [](ItemInstance *state) {
					 SearchAndDestroy(static_cast<InfantryState *>(state));
				 }},
				{Orders::Order::Standing, [](ItemInstance *state) {
					 Standing(static_cast<InfantryState *>(state));
				 }},
				{Orders::Order::Stand, [](ItemInstance * /*state*/) {
					 Stand();
				 }},
				{Orders::Order::Destroyed, [](ItemInstance *state) {
					 Destroyed(state);
				 }},
				{Orders::Order::Load, [](ItemInstance *state) {
					 Load(static_cast<InfantryState *>(state));
				 }},
			};
		}

		Log::Success("Infantry Init created");
	}

	MODULE_API ItemInstance *Awake(const String &name)
	{
		Log::Success("Infantry Awake " + name);

		if (name == "soldier")
		{
			return globalItem = new SoldierState();
		}
		if (name == "grenadier")
		{
			return globalItem = new GrenadierState();
		}
		if (name == "rocketeer")
		{
			return globalItem = new RocketeerState();
		}
		if (name == "scientist")
		{
			return globalItem = new ScientistState();
		}

		return nullptr;
	}

	MODULE_API void Create(
		Vector<sf::Sprite *> *spritesRef,
		Vector<sf::Texture *> *texturesRef)
	{
		Log::Info("Creating the Infantry's texture and sprite");

		infantries[globalItem->GetUid()] = std::make_unique<Infantries>(globalItem);

		String asset = globalItem->GetTeam() + "/" + globalItem->GetType() + "/" + globalItem->GetName();

		for (int i = 0; i < globalItem->GetFrames(); i++)
		{
			String filename = "Resources/images/" + asset + "/" + std::to_string(i) + ".png";

			ImageLoader::Load(filename, globalItem, spritesRef, texturesRef);
		}

		globalItem->SetLife(globalItem->GetHitPoints());
		globalItem->SetArmy(true);

		WorldState &world = WorldState::GetInstance();

		if (globalItem->IsTraining())
		{
			int index = LookUp::Get(world.GetPrimaryItem(globalItem->GetTeam(), "barracks"));

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
				}
			}
			else
			{
				Log::Error("Barracks item not found!");
			}
		}

		Physics &physics = Physics::GetInstance();

		globalItem->AddToGrid(world.currentTerrainMapPassableGrid, physics.GetGridTracker());

		CreatePolygons();
		RegisterToQuadTree(globalItem->GetGroups());
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
		auto orderFunction = orderMap.find(itemInstance->GetOrders()->order);

		if (orderFunction != orderMap.end())
		{
			orderFunction->second(itemInstance);
		}
	}

	MODULE_API void Draw(InfantryState *itemState, Vector<sf::Sprite *> *spritesRef)
	{
		if (itemState->GetHidden())
		{
			return;
		}

		if (itemState->GetOrders()->order != Orders::Order::Stand &&
			itemState->GetOrders()->order != Orders::Order::Standing &&
			itemState->GetOrders()->order != Orders::Order::Firing)
		{
			Animate(itemState);
			AnimatePolygons(itemState);
		}

		infantries[itemState->GetUid()]->Draw(itemState, spritesRef);
	}

	MODULE_API void Update(ItemInstance *itemInstance, std::vector<sf::Sprite *> *spritesRef)
	{
		WorldState &world = WorldState::GetInstance();

		const bool isVisible = itemInstance->GetTeam() == WorldState::GetInstance().GetTeam() || itemInstance->isVisible();

		if (HoverFromCenter(itemInstance, itemInstance->GetRadius()) && isVisible)
		{
			if (itemInstance->GetHidden())
			{
				return;
			}

			world.SetItemUidThatIsUnderCursor(itemInstance->GetUid());

			if (world.GetTeam() == itemInstance->GetTeam())
			{
				world.SetItemUnderCursor(true);

				if (world.IsLeftClicked())
				{
					world.selected.emplace_back(itemInstance->GetUid());
				}
			}
			else
			{
				if (!world.selected.empty())
				{
					// Any one of the selection being able to shoot is enough. This asked
					// only the first, so a box that happened to pick up a prospector or a
					// scientist first offered no attack cursor at all and none of the
					// soldiers behind it could be ordered onto anything.
					bool anyCanAttack = false;

					for (int selectedUid : world.selected)
					{
						const int selectedIndex = LookUp::Get(selectedUid);

						if (selectedIndex != -1 && world.items[selectedIndex] &&
							world.items[selectedIndex]->CanAttack())
						{
							anyCanAttack = true;
							break;
						}
					}

					if (anyCanAttack)
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

		int frame = itemInstance->GetFrame();

		if (spritesRef == nullptr || frame < 0 || frame >= static_cast<int>(spritesRef->size()))
		{
			return;
		}

		(*spritesRef)[frame]->setPosition(
			sf::Vector2f(
				((itemInstance->GetX() * Globals::grid_size) + static_cast<float>(world.GetMapXOffset())),
				((itemInstance->GetY() * Globals::grid_size) + static_cast<float>(world.GetMapYOffset()))));

		auto *infantryState = static_cast<InfantryState *>(itemInstance);

		itemInstance->SetFrame(
			(static_cast<int>(round(itemInstance->GetDirection())) % itemInstance->GetDirections()) + infantryState->animationOffset);

		infantries[itemInstance->GetUid()]->Update(itemInstance, spritesRef);
	}

	MODULE_API void Delete(int uid)
	{
		Log::Clean("Current size of the infantries map: " + std::to_string(infantries.size()));
		Log::Clean("Delete infantries uid: " + std::to_string(uid));

		auto it = infantries.find(uid);
		if (it != infantries.end())
		{
			infantries.erase(it);
		}
	}

	MODULE_API void Destroy(const String &name)
	{
		Log::Print(StringConcat("Destroying static infantry { ", name, " }"));

		near.reset();

		Log::Success("Destroying static infantry: " + name);
	}
}

void Action(ItemInstance *itemInstance)
{
	Orders::Order order = Orders::Order::Stand;

	WorldState &world = WorldState::GetInstance();

	if (world.IsEnemyItemUnderCursor())
	{
		order = Orders::Order::Attack;
	}
	else
	{
		int index = LookUp::Get(world.GetItemUidThatIsUnderCursor());

		if (index != -1)
		{
			ItemInstance *targetInstance = world.items[index].get();

			if (itemInstance->IsLoadable() && targetInstance->CanLoad())
			{
				itemInstance->SetTargetUid(targetInstance->GetUid());
				order = Orders::Order::Load;
			}
		}
	}

	itemInstance->SetOrders(order);
}

void Attack(ItemInstance *itemInstance)
{
	if (!itemInstance->CanAttack())
	{
		itemInstance->GetOrders()->id = -1;
		itemInstance->SetOrders(Orders::Order::Stand);
		return;
	}

	WorldState &world = WorldState::GetInstance();
	int targetIndex = LookUp::Get(world.GetItemUidThatIsUnderCursor());

	if (targetIndex < 0 || targetIndex >= static_cast<int>(world.items.size()))
	{
		itemInstance->SetTargetUid(0);
		return;
	}

	Physics &physics = Physics::GetInstance();

	itemInstance->RemoveFromGrid(world.currentTerrainMapPassableGrid, physics.GetGridTracker());

	ItemInstance *targetInstance = world.items[targetIndex].get();

	itemInstance->SetState(ItemStates::Attacking);

	itemInstance->SetTargetUid(targetInstance->GetUid());

	itemInstance->GetOrders()->toX = targetInstance->GetX();
	itemInstance->GetOrders()->toY = targetInstance->GetY();
	itemInstance->SetOrders(Orders::Order::MoveTo);
}

void Move(ItemInstance *itemInstance)
{
	Log::Warning("Move " + std::to_string(itemInstance->GetUid()));

	WorldState &world = WorldState::GetInstance();
	Physics &physics = Physics::GetInstance();

	itemInstance->RemoveFromGrid(world.currentTerrainMapPassableGrid, physics.GetGridTracker());
	static_cast<InfantryState *>(itemInstance)->RemoveTacticalGrid(
		itemInstance->GetUid(), world.currentTerrainMapPassableGrid, physics.GetGridTracker());

	itemInstance->SetOrders(Orders::Order::MoveTo);
}

void MoveTo(ItemInstance *itemInstance)
{
	Log::Info("MoveTo");

	auto *infantry = static_cast<InfantryState *>(itemInstance);

	float destX = 0.0;
	float destY = 0.0;

	WorldState &world = WorldState::GetInstance();
	if (itemInstance->GetState() == ItemStates::Attacking)
	{
		Physics &physics = Physics::GetInstance();

		Log::Debug("RemoveFromGrid for Target");
		int targetIndex = LookUp::Get(itemInstance->GetTargetUid());

		if (targetIndex != -1)
		{
			world.items[targetIndex]->RemoveFromGrid(
				world.currentTerrainMapPassableGrid,
				physics.GetGridTracker());

			SetPath(infantry, world.items[targetIndex]->GetCenterX(), world.items[targetIndex]->GetCenterY());

			Log::Debug("(Re)AddToGrid for Target");
			world.items[targetIndex]->AddToGrid(
				world.currentTerrainMapPassableGrid,
				physics.GetGridTracker());
		}
	}
	else
	{
		destX = itemInstance->GetOrders()->toX;
		destY = itemInstance->GetOrders()->toY;

		SpreadDestination(destX, destY);

		itemInstance->GetOrders()->toX = destX;
		itemInstance->GetOrders()->toY = destY;

		SetPath(infantry, itemInstance->GetOrders()->toX, itemInstance->GetOrders()->toY);
	}

	bool pathEmpty = infantry->path.empty();

	if (!pathEmpty)
	{
		infantry->hasNextStep = false;

		// Claim the cell this one will stop on. Vehicles have always done it;
		// infantry never did, so a whole wave walked to the same spot and stood
		// in it -- and a grenadier and a truck share a reach, which put them on
		// the same ring, on the same approach, at the same point. The traversal
		// rule passes a cell only when it is completely clear, so one booking is
		// enough to bend the next unit's path around it.
		if (itemInstance->GetState() == ItemStates::Attacking)
		{
			const int targetIndex = LookUp::Get(itemInstance->GetTargetUid());

			if (targetIndex != -1)
			{
				SetTacticalCoordinates(
					itemInstance,
					infantry->path,
					world.items[targetIndex]->GetCenterX(),
					world.items[targetIndex]->GetCenterY(),
					infantry->GetSight() +
						static_cast<int>(world.items[targetIndex]->GetOuterSight()));
			}
		}

		itemInstance->SetOrders(Orders::Order::Turning);
	}
	else
	{
		itemInstance->SetOrders(Orders::Order::Standing);
	}
}

void Turning(InfantryState *itemInstance)
{
	Log::Info("Turning");

	itemInstance->SetOrders(Orders::Order::Moving);
}

void TurnToFire(InfantryState *itemInstance)
{
	itemInstance->SetState(ItemStates::Firing);
	itemInstance->SetOrders(Orders::Order::Firing);
}

void Firing(InfantryState *itemInstance)
{
	int targetIndex = LookUp::Get(itemInstance->GetTargetUid());

	if (targetIndex == -1)
	{
		Log::Error("Target State is nullptr");
		itemInstance->SetOrders(Orders::Order::Standing);
		return;
	}

	if ((itemInstance->reloadTimeLeft % itemInstance->GetReloadTime()) == 0)
	{
		WorldState &world = WorldState::GetInstance();

		if (world.items[targetIndex]->GetLife() <= 0)
		{
			itemInstance->reloadTimeLeft = 0;
			itemInstance->SetOrders(Orders::Order::Standing);
			return;
		}

		float dx = world.items[targetIndex]->GetCenterX() - itemInstance->GetX();
		float dy = world.items[targetIndex]->GetCenterY() - itemInstance->GetY();

		// Sight plus the target's own reach, the way getTargetOuterSight does it
		// in the client. Without the second term a unit at the wall of anything
		// bigger than its sight is forever out of range and stands there.
		const float range = static_cast<float>(itemInstance->GetSight()) +
							world.items[targetIndex]->GetOuterSight();

		if (((dx * dx) + (dy * dy)) < (range * range))
		{
			itemInstance->SetOrders(Orders::Order::Fire);
		}
		else
		{
			itemInstance->SetOrders(Orders::Order::Standing);
		}
	}

	itemInstance->reloadTimeLeft++;
}

void Fire(InfantryState *itemInstance)
{
	WorldState &world = WorldState::GetInstance();

	String projectileName = itemInstance->GetWeapon();

	auto it = world.projectileRegistry.find(projectileName);
	if (it == world.projectileRegistry.end())
	{
		Log::Warning("Fire: no projectile registered for weapon '" + projectileName + "'");
		itemInstance->SetOrders(Orders::Order::Standing);

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
		bullet->targetX = world.items[targetIndex]->GetCenterX();
		bullet->targetY = world.items[targetIndex]->GetCenterY();
		bullet->targetUid = world.items[targetIndex]->GetUid();
	}

	world.projectiles[projectileName].emplace_back(std::move(bullet));

	itemInstance->SetOrders(Orders::Order::Firing);
}

void Moving(InfantryState *itemInstance)
{
	WorldState &world = WorldState::GetInstance();
	auto &path = itemInstance->path;

	if (!itemInstance->hasNextStep)
	{
		if (path.empty())
		{
			Log::Warning("Path is empty. Cannot initialize next step.");
			itemInstance->SetOrders(Orders::Order::Stand);
			return;
		}

		const auto &nextPathStep = path.back();
		itemInstance->nextStep.x = nextPathStep.x;
		itemInstance->nextStep.y = nextPathStep.y;
		itemInstance->hasNextStep = true;
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
			}
			else
			{
				itemInstance->hasNextStep = false;
				itemInstance->SetOrders(Orders::Order::Standing);
				return;
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

		float movement = itemInstance->GetSpeed() * (1.0f / 64.0f) * world.GetDeltaTime();

		float angleRadians = -(itemInstance->GetDirection() / static_cast<float>(itemInstance->GetDirections())) * 2.0f * PI;

		float moveX = -(movement * std::sin(angleRadians));
		float moveY = -(movement * std::cos(angleRadians));

		itemInstance->SetLastMovementX(moveX);
		itemInstance->SetLastMovementY(moveY);

		itemInstance->SetX(itemInstance->GetX() + moveX);
		itemInstance->SetY(itemInstance->GetY() + moveY);
	}

	Steering(itemInstance);
}

void Attacked(InfantryState *itemInstance)
{
	if (itemInstance == nullptr)
	{
		Log::Warning("Attacked() called with null item instance.");
		return;
	}

	if (itemInstance->GetState() != ItemStates::Attacking && itemInstance->CanAttack())
	{
		itemInstance->SetState(ItemStates::Attacking);

		int targetUid = itemInstance->GetOrders()->target_uid;
		int targetIndex = LookUp::Get(targetUid);

		WorldState &world = WorldState::GetInstance();
		if (targetIndex >= 0 && targetIndex < static_cast<int>(world.items.size()))
		{
			ItemInstance *targetState = world.items[targetIndex].get();

			if (targetState != nullptr)
			{
				itemInstance->SetTargetUid(targetState->GetUid());
				itemInstance->SetOrders(Orders::Order::MoveTo);

				return;
			}

			Log::Warning("Attacked: targetState is null for UID: " + std::to_string(targetUid));
		}
		else
		{
			Log::Warning("Attacked: Invalid target index for UID: " + std::to_string(targetUid));
		}
	}

	itemInstance->SetOrders(Orders::Order::Standing);
}

void Search(InfantryState *itemInstance)
{
	WorldState &world = WorldState::GetInstance();
	Log::Info("Search: " + std::to_string(world.GetMapGridWidth()) + " " + std::to_string(world.GetMapGridHeight()));
	Log::Info("itemState->GetUid: " + std::to_string(itemInstance->GetUid()));

	itemInstance->GetOrders()->toX = static_cast<float>(Random::get(0, world.GetMapGridWidth()));
	itemInstance->GetOrders()->toY = static_cast<float>(Random::get(0, world.GetMapGridHeight()));

	itemInstance->SetOrders(Orders::Order::MoveTo);
}

void SearchAndDestroy(InfantryState *itemInstance)
{
	Log::Info("SearchAndDestroy check for: " + itemInstance->GetName());

	WorldState &world = WorldState::GetInstance();

	for (auto &itemPtr : world.items)
	{
		if (!itemPtr || itemPtr.get() == itemInstance)
		{
			continue;
		}

		if (itemPtr->GetTeam() == itemInstance->GetTeam())
		{
			continue;
		}

		itemInstance->SetState(ItemStates::Attacking);
		itemInstance->SetTargetUid(itemPtr->GetUid());

		itemInstance->GetOrders()->toX = itemPtr->GetX() * Globals::grid_size;
		itemInstance->GetOrders()->toY = itemPtr->GetY() * Globals::grid_size;

		Log::Info("Target Acquired: " +
				  std::to_string(itemInstance->GetOrders()->toX) + " " +
				  std::to_string(itemInstance->GetOrders()->toY));

		itemInstance->SetOrders(Orders::Order::MoveTo);
		return;
	}

	itemInstance->SetTargetUid(-1);
	itemInstance->SetOrders(Orders::Order::Standing);
}

void Standing(InfantryState *itemInstance)
{
	Log::Info("Standing");

	WorldState &world = WorldState::GetInstance();
	Physics &physics = Physics::GetInstance();

	itemInstance->AddToGrid(
		world.currentTerrainMapPassableGrid,
		physics.GetGridTracker());

	itemInstance->RemoveTacticalGrid(
		itemInstance->GetUid(),
		world.currentTerrainMapPassableGrid,
		physics.GetGridTracker());

	if (itemInstance->GetState() == ItemStates::Attacking && itemInstance->CanAttack())
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

void Destroyed(ItemInstance *itemInstance)
{
	WorldState &world = WorldState::GetInstance();
	Physics &physics = Physics::GetInstance();

	String removeItem = StringConcat("uid:", itemInstance->GetUid());
	world.gameEvents.emplace_back(UIAction::RemoveGameItem, removeItem);

	itemInstance->RemoveFromGrid(world.currentTerrainMapPassableGrid, physics.GetGridTracker());

	static_cast<InfantryState *>(itemInstance)->RemoveTacticalGrid(
		itemInstance->GetUid(),
		world.currentTerrainMapPassableGrid,
		physics.GetGridTracker());
}

void Animate(InfantryState *itemInstance)
{
	if ((infantries[itemInstance->GetUid()]->animationSpeed % itemInstance->animationSpeedLimit) == 0)
	{
		if (infantries[itemInstance->GetUid()]->animationCount < itemInstance->animationLimit)
		{
			infantries[itemInstance->GetUid()]->animationCount++;
		}
		else
		{
			if (itemInstance->animationOffset == 0)
			{
				itemInstance->animationOffset = 8;
			}
			else
			{
				itemInstance->animationOffset = 0;
			}
		}
	}

	infantries[itemInstance->GetUid()]->animationSpeed++;
}

void SetPath(InfantryState *itemInstance, float toX, float toY)
{
	WorldState &world = WorldState::GetInstance();

	bool reachable = false;
	Point destination = Navigation::NearestOpenCell(
		world.currentTerrainMapPassableGrid,
		{toX, toY},
		reachable);

	if (!reachable)
	{
		itemInstance->path.clear();

		return;
	}

	itemInstance->path = Navigation::GetInstance().GetPath(
		world.currentTerrainMapPassableGrid,
		{itemInstance->GetX(), itemInstance->GetY()},
		destination,
		itemInstance->GetCellCollisionMode(),
		PathfindingAlgorithm::AStar,
		HeuristicType::Euclidean,
		TraversalType::Soft);

	Log::Warning((int)itemInstance->path.size());
}

void CreatePolygons()
{
	near = std::make_unique<Boundary>();
}

void RegisterToQuadTree(const Set<String> &groups)
{
	Physics &physics = Physics::GetInstance();

	physics.Add(groups);
	physics.Show();
}

void AnimatePolygons(ItemInstance *itemInstance)
{
	float translateX = itemInstance->GetX();
	float translateY = itemInstance->GetY();

	near->x = translateX - 5.0f;
	near->y = translateY - 5.0f;
	near->w = near->x + 10.0f;
	near->h = near->y + 10.0f;

	Log::Info("Near X: " + std::to_string(near->x) + ", Near Y : " + std::to_string(near->y) +
				  ", Near W: " + std::to_string(near->w) + ", Near H : " + std::to_string(near->h),
			  false);
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

	static_cast<InfantryState *>(itemInstance)->AddTacticalGrid(itemInstance->GetUid(), static_cast<int>(pathIndex), path, static_cast<int>(itemInstance->GetRadius()) / 20, world.currentTerrainMapPassableGrid, physics.GetGridTracker());

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

	Vector<PointUID> foundUnits = physics.Find(itemInstance->GetGroups(), nearbyRange);

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
			nearbyItems.emplace_back(world.items[index].get());
		}

		Vector<ItemInstance *> collidedBodyItems = Detect(itemInstance, nearbyItems);

		for (auto &collidedItem : collidedBodyItems)
		{
			// A wave converges on one target, so without this every unit walks to
			// the same place and stands in it. Anything that reaches a friendly
			// already stopped on the same target has arrived too. The order-group
			// rule below cannot cover this: the AI stamps no order id, so its units
			// all carry 0 and never match each other.
			const Orders::Order theirs = collidedItem->GetOrders()->order;

			const bool theyHaveArrived =
				theirs == Orders::Order::Stand || theirs == Orders::Order::Standing ||
				theirs == Orders::Order::TurnToFire || theirs == Orders::Order::Firing ||
				theirs == Orders::Order::Fire;

			if (itemInstance->GetState() == ItemStates::Attacking && theyHaveArrived &&
				collidedItem->GetTeam() == itemInstance->GetTeam() &&
				collidedItem->GetTargetUid() == itemInstance->GetTargetUid())
			{
				// Only if this one can shoot from where it is standing. Each weapon
				// has its own reach, so a wave arrives as rings: a tank halts a dozen
				// cells further out than a soldier needs to be. Standing the soldier
				// down where it met the tank leaves it short of the wall for good.
				const int index = LookUp::Get(itemInstance->GetTargetUid());

				if (index != -1)
				{
					const float reach = static_cast<float>(itemInstance->GetSight()) +
										world.items[index]->GetOuterSight();

					const float offsetX = world.items[index]->GetCenterX() - itemInstance->GetX();
					const float offsetY = world.items[index]->GetCenterY() - itemInstance->GetY();

					if (((offsetX * offsetX) + (offsetY * offsetY)) < (reach * reach))
					{
						itemInstance->SetOrders(Orders::Order::Standing);
						return;
					}
				}
			}

			// Both have to actually be in the same commanded group. An order id of 0
			// is what a unit has when nobody has grouped it -- every AI unit, and
			// anything freshly built -- so matching on it stood down any unit that
			// so much as brushed another one parked at the factory.
			if (collidedItem &&
				(itemInstance->GetOrders()->id > 0) &&
				(itemInstance->GetOrders()->id == collidedItem->GetOrders()->id) &&
				(collidedItem->GetOrders()->order == Orders::Order::Stand || collidedItem->GetOrders()->order == Orders::Order::Standing))
			{
				Log::Warning("Standing Collision Detected");

				itemInstance->SetOrders(Orders::Order::Standing);
				return;
			}
		}
	}
}

Vector<ItemInstance *> Detect(ItemInstance *itemInstance, const Vector<ItemInstance *> &nearByItems)
{
	Vector<ItemInstance *> collidedItems;

	for (const auto &collisionItem : nearByItems)
	{
		// Infantry walk through each other, which is the client's own rule -- its
		// infantry steering drops every "infantry" from the list before testing. They
		// do stop for vehicles, and for aircraft they should not: those pass overhead
		// and never build a ground polygon to test against.
		if (!collisionItem || collisionItem == itemInstance ||
			collisionItem->GetType() == "infantry" || collisionItem->IsAircraft())
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

void SpreadDestination(float &x, float &y)
{
	Vector2D acc(0.0f, 0.0f);
	Vector2D vel(0.0f, 0.0f);

	const auto originalX = x;
	const auto originalY = y;

	Vector2D pos(originalX, originalY);

	vel = Vector2D::Random();

	float norm = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);

	vel.SetMagnitude(norm + 0.5f);

	pos += vel;
	vel += acc;

	x = pos.x;
	y = pos.y;

	int endX = static_cast<int>(std::floor(x));
	int endY = static_cast<int>(std::floor(y));

	WorldState &world = WorldState::GetInstance();

	if (endX < 0 || endY < 0 ||
		endY >= static_cast<int>(world.currentTerrainMapPassableGrid.size()) ||
		endX >= static_cast<int>(world.currentTerrainMapPassableGrid[0].size()))
	{
		x = originalX;
		y = originalY;
		return;
	}

	int cell = world.currentTerrainMapPassableGrid[endY][endX];

	if (cell == Flags::CELL_COLLISION_MODE_FULL)
	{
		x = originalX;
		y = originalY;
		return;
	}

	if (cell > Flags::CELL_COLLISION_MODE_SOFT)
	{
		x = originalX;
		y = originalY;
		return;
	}
}

void Load(InfantryState *itemInstance)
{
	Log::Info("Load");

	WorldState &world = WorldState::GetInstance();
	int targetIndex = LookUp::Get(itemInstance->GetTargetUid());

	if (targetIndex != -1)
	{
		ItemInstance *vehicle = world.items[targetIndex].get();

		auto *thresholdPtr = vehicle->GetExtra<int>(ItemInstance::ItemProperty::LoadThreshold);
		auto *loadedUIDs = vehicle->GetExtra<std::vector<int>>(ItemInstance::ItemProperty::LoadedUIDs);

		int loadThreshold = thresholdPtr ? *thresholdPtr : 0;
		int loadCount = loadedUIDs ? static_cast<int>(loadedUIDs->size()) : 0;

		auto *capacityPtr = vehicle->GetExtra<int>(ItemInstance::ItemProperty::LoadCapacity);
		int loadCapacity = capacityPtr ? *capacityPtr : 5;

		Log::Crash("Load Threshold: " + std::to_string(loadThreshold) +
				   ", Load Count: " + std::to_string(loadCount) +
				   ", Load Capacity: " + std::to_string(loadCapacity));

		if (std::pow(vehicle->GetX() - itemInstance->GetX(), 2) + pow(vehicle->GetY() - itemInstance->GetY(), 2) < loadThreshold)
		{
			if (loadedUIDs && itemInstance->IsLoadable() && loadCount < loadCapacity)
			{
				Log::Crash("Loading unit into APC");
				Physics &physics = Physics::GetInstance();

				std::erase_if(world.selected, [&](int selected) {
					return selected == itemInstance->GetUid();
				});

				itemInstance->SetOrders(Orders::Order::Stand);
				itemInstance->SetX(Globals::OFFSCREEN_X);
				itemInstance->SetY(Globals::OFFSCREEN_Y);
				itemInstance->RemoveFromGrid(world.currentTerrainMapPassableGrid, physics.GetGridTracker());
				Log::Info(itemInstance->GetUid());

				loadedUIDs->emplace_back(itemInstance->GetUid());

				itemInstance->SetHidden(true);
			}
		}
	}
}

void TestSearch()
{
}
} // namespace TGX
