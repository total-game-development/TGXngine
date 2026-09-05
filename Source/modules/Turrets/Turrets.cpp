#include "Turrets.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include "Enums.h"
#include "Flags.h"
#include "Globals.h"
#include "ImageLoader.h"
#include "Logs.h"
#include "Lookup.h"
#include "Orders.h"
#include "StringUtils.hpp"
#include "TurretStates.h"
#include "module_interface.h"

namespace TGX
{
namespace
{
bool IsEngageable(const TurretState *turret, const ItemInstance *candidate)
{
	if (candidate == nullptr || candidate == turret)
	{
		return false;
	}

	if (!candidate->IsAttackable() || candidate->GetLife() <= 0 || candidate->GetHidden())
	{
		return false;
	}

	if (candidate->GetTeam() == turret->GetTeam())
	{
		return false;
	}

	return candidate->IsAircraft() ? turret->canTargetAir : turret->canTargetLand;
}

int FindTarget(const TurretState *turret)
{
	WorldState &world = WorldState::GetInstance();

	const auto reach = static_cast<float>(turret->GetSight());
	const float reachSq = reach * reach;

	int bestIndex = -1;
	float bestDistanceSq = std::numeric_limits<float>::max();
	int bestUid = 0;

	for (size_t i = 0; i < world.items.size(); i++)
	{
		const ItemInstance *candidate = world.items[i].get();

		if (!IsEngageable(turret, candidate))
		{
			continue;
		}

		const float dx = candidate->GetCenterX() - turret->GetCenterX();
		const float dy = candidate->GetCenterY() - turret->GetCenterY();

		const float distanceSq = (dx * dx) + (dy * dy);

		if (distanceSq > reachSq)
		{
			continue;
		}

		if (distanceSq < bestDistanceSq ||
			(distanceSq == bestDistanceSq && candidate->GetUid() < bestUid))
		{
			bestIndex = static_cast<int>(i);
			bestDistanceSq = distanceSq;
			bestUid = candidate->GetUid();
		}
	}

	return bestIndex;
}

constexpr float traversePerFrame = 0.35f;
constexpr float aimTolerance = 0.5f;

void TrackTarget(TurretState *turret, const ItemInstance *target)
{
	const int directions = std::max(1, turret->GetDirections());

	const float bearing = FindAngle(
		target->GetCenterX(), target->GetCenterY(),
		turret->GetCenterX(), turret->GetCenterY(),
		directions);

	const float offset = AngleDiff(turret->GetDirection(), bearing, directions);

	if (std::abs(offset) <= traversePerFrame)
	{
		turret->SetDirection(bearing);
		return;
	}

	const float heading = turret->GetDirection() - std::copysign(traversePerFrame, offset);

	turret->SetDirection(WrapDirection(heading, directions));
}

void Stand(TurretState *turret)
{
	turret->SetTargetUid(-1);
	turret->SetState(ItemStates::Stand);
	turret->SetOrders(Orders::Order::Standing);
}

void Standing(TurretState *turret)
{
	const int targetIndex = FindTarget(turret);

	if (targetIndex == -1)
	{
		return;
	}

	WorldState &world = WorldState::GetInstance();

	turret->SetTargetUid(world.items[targetIndex]->GetUid());
	turret->SetState(ItemStates::Firing);
	turret->SetOrders(Orders::Order::Firing);
}

void Firing(TurretState *turret)
{
	const int targetIndex = LookUp::Get(turret->GetTargetUid());

	WorldState &world = WorldState::GetInstance();

	if (targetIndex < 0 || targetIndex >= static_cast<int>(world.items.size()))
	{
		Stand(turret);
		return;
	}

	const ItemInstance *target = world.items[targetIndex].get();

	if (target == nullptr || target->GetLife() <= 0)
	{
		turret->reloadTimeLeft = 0;
		Stand(turret);
		return;
	}

	const float dx = target->GetCenterX() - turret->GetCenterX();
	const float dy = target->GetCenterY() - turret->GetCenterY();

	const auto reach = static_cast<float>(turret->GetSight());

	if (((dx * dx) + (dy * dy)) > (reach * reach))
	{
		turret->reloadTimeLeft = 0;
		Stand(turret);
		return;
	}

	TrackTarget(turret, target);

	if (turret->GetReloadTime() > 0 && (turret->reloadTimeLeft % turret->GetReloadTime()) == 0)
	{
		const int directions = std::max(1, turret->GetDirections());
		const float bearing = FindAngle(
			target->GetCenterX(), target->GetCenterY(),
			turret->GetCenterX(), turret->GetCenterY(),
			directions);

		if (std::abs(AngleDiff(turret->GetDirection(), bearing, directions)) > aimTolerance)
		{
			return;
		}

		turret->SetOrders(Orders::Order::Fire);
	}

	turret->reloadTimeLeft++;
}

void Fire(TurretState *turret)
{
	WorldState &world = WorldState::GetInstance();

	const String &weaponName = turret->GetWeapon();

	auto weapon = world.projectileRegistry.find(weaponName);
	if (weapon == world.projectileRegistry.end())
	{
		Log::Warning("Turret has no such weapon registered: " + weaponName);
		Stand(turret);
		return;
	}

	const int targetIndex = LookUp::Get(turret->GetTargetUid());

	if (targetIndex < 0 || targetIndex >= static_cast<int>(world.items.size()))
	{
		Stand(turret);
		return;
	}

	const ItemInstance *target = world.items[targetIndex].get();

	if (target == nullptr)
	{
		Stand(turret);
		return;
	}

	auto round = weapon->second->Clone();

	round->uid = turret->GetUid();
	round->x = turret->GetCenterX();
	round->y = turret->GetCenterY();
	round->direction = turret->GetDirection();
	round->SetTarget(target->GetUid(), target->GetCenterX(), target->GetCenterY());

	world.projectiles[weaponName].emplace_back(std::move(round));

	turret->SetOrders(Orders::Order::Firing);
}

void Attacked(TurretState *turret)
{
	if (turret->GetState() == ItemStates::Firing)
	{
		return;
	}

	WorldState &world = WorldState::GetInstance();

	const int attackerIndex = LookUp::Get(turret->GetOrders()->target_uid);

	if (attackerIndex < 0 || attackerIndex >= static_cast<int>(world.items.size()))
	{
		Stand(turret);
		return;
	}

	const ItemInstance *attacker = world.items[attackerIndex].get();

	if (!IsEngageable(turret, attacker))
	{
		Stand(turret);
		return;
	}

	const float dx = attacker->GetCenterX() - turret->GetCenterX();
	const float dy = attacker->GetCenterY() - turret->GetCenterY();

	const auto reach = static_cast<float>(turret->GetSight());

	if (((dx * dx) + (dy * dy)) > (reach * reach))
	{
		Stand(turret);
		return;
	}

	turret->SetTargetUid(attacker->GetUid());
	turret->SetState(ItemStates::Firing);
	turret->SetOrders(Orders::Order::Firing);
}

void Destroyed(TurretState *turret)
{
	WorldState &world = WorldState::GetInstance();

	const auto &grid = turret->passableGrid;

	for (size_t y = 0; y < grid.size(); ++y)
	{
		for (size_t x = 0; x < grid[y].size(); ++x)
		{
			const int gridX = static_cast<int>(turret->GetX()) + static_cast<int>(x);
			const int gridY = static_cast<int>(turret->GetY()) + static_cast<int>(y);

			if (gridY < 0 || gridY >= world.GetMapGridHeight() || gridX < 0 || gridX >= world.GetMapGridWidth())
			{
				continue;
			}

			world.currentTerrainMapPassableGrid[gridY][gridX] = Flags::CELL_COLLISION_MODE_OFF;
		}
	}

	const String &owner = turret->GetTeam();

	if (turret->GetPowerUsage() < 0)
	{
		world.SetPowerTotal(owner, world.GetPowerTotal(owner) + turret->GetPowerUsage());
	}
	else
	{
		world.SetPowerUsage(owner, world.GetPowerUsage(owner) - turret->GetPowerUsage());
	}

	String removeItem = StringConcat("uid:", turret->GetUid());
	world.gameEvents.emplace_back(UIAction::RemoveGameItem, removeItem);
}
} // namespace

extern "C"
{
	MODULE_API void OutputTest()
	{
		Log::Print("This is the turrets output test");
	}

	MODULE_API void Init()
	{
		if (orderMap.empty())
		{
			orderMap = {
				{Orders::Order::Stand, [](ItemInstance *state) {
					 Stand(static_cast<TurretState *>(state));
				 }},
				{Orders::Order::Standing, [](ItemInstance *state) {
					 Standing(static_cast<TurretState *>(state));
				 }},
				{Orders::Order::Firing, [](ItemInstance *state) {
					 Firing(static_cast<TurretState *>(state));
				 }},
				{Orders::Order::Fire, [](ItemInstance *state) {
					 Fire(static_cast<TurretState *>(state));
				 }},
				{Orders::Order::Attacked, [](ItemInstance *state) {
					 Attacked(static_cast<TurretState *>(state));
				 }},
				{Orders::Order::Destroyed, [](ItemInstance *state) {
					 Destroyed(static_cast<TurretState *>(state));
				 }},
			};
		}

		Log::Success("Turrets Init created");
	}

	MODULE_API ItemInstance *Awake(const String &name)
	{
		Log::Info("Awake " + name + " Turret");

		if (name == "laser-tower")
		{
			return globalItem = new LaserTowerState();
		}
		if (name == "missile-turret")
		{
			return globalItem = new MissileTurretState();
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

		const String asset = globalItem->GetTeam() + "/" + globalItem->GetType() + "/" + globalItem->GetName();

		for (int i = 0; i < globalItem->GetFrames(); i++)
		{
			String filename = "Resources/images/" + asset + "/" + std::to_string(i) + ".png";
			ImageLoader::Load(filename, globalItem, spritesRef, texturesRef);
		}

		if (!texturesRef->empty())
		{
			globalItem->SetWidth(static_cast<int>((*texturesRef)[0]->getSize().x));
			globalItem->SetHeight(static_cast<int>((*texturesRef)[0]->getSize().y));
		}

		turrets[globalItem->GetUid()] = std::make_unique<Turrets>(globalItem);

		auto *turretState = static_cast<TurretState *>(globalItem);

		WorldState &world = WorldState::GetInstance();

		if (world.IsPlacement())
		{
			world.SetPlacement(false);
		}

		const auto &grid = turretState->passableGrid;

		for (size_t y = 0; y < grid.size(); ++y)
		{
			for (size_t x = 0; x < grid[y].size(); ++x)
			{
				const int gridX = static_cast<int>(globalItem->GetX()) + static_cast<int>(x);
				const int gridY = static_cast<int>(globalItem->GetY()) + static_cast<int>(y);

				if (gridY < 0 || gridY >= world.GetMapGridHeight() || gridX < 0 || gridX >= world.GetMapGridWidth())
				{
					continue;
				}

				world.currentTerrainMapPassableGrid[gridY][gridX] = Flags::CELL_COLLISION_MODE_HARD;
			}
		}

		globalItem->SetLife(globalItem->GetHitPoints());
		globalItem->SetBuildable(true);
		globalItem->SetOrders(Orders::Order::Standing);

		world.SetPrimaryItems(globalItem->GetTeam(), globalItem->GetName(), globalItem->GetUid());

		const String &owner = globalItem->GetTeam();

		if (turretState->GetPowerUsage() < 0)
		{
			world.SetPowerTotal(owner, world.GetPowerTotal(owner) - turretState->GetPowerUsage());
		}
		else
		{
			world.SetPowerUsage(owner, world.GetPowerUsage(owner) + turretState->GetPowerUsage());
		}

		Log::Success("Turret " + globalItem->GetName() + " created successfully");
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

	MODULE_API void Draw(ItemInstance *itemInstance, Vector<sf::Sprite *> *spritesRef)
	{
		auto it = turrets.find(itemInstance->GetUid());

		if (it != turrets.end())
		{
			it->second->Draw(itemInstance, spritesRef);
		}
	}

	MODULE_API void Update(ItemInstance *itemInstance, Vector<sf::Sprite *> *spritesRef)
	{
		if (itemInstance->GetHidden())
		{
			return;
		}

		WorldState &world = WorldState::GetInstance();

		if (HoverFromCorner(itemInstance))
		{
			world.SetItemUnderCursor(true);
			world.SetItemUidThatIsUnderCursor(itemInstance->GetUid());

			if (world.IsLeftClicked())
			{
				itemInstance->SetSelected(true);
				world.selected.emplace_back(itemInstance->GetUid());
			}
		}

		int frame = itemInstance->GetFrame();

		if (spritesRef == nullptr || frame < 0 || frame >= static_cast<int>(spritesRef->size()))
		{
			return;
		}

		(*spritesRef)[frame]->setPosition(
			((itemInstance->GetX() * Globals::grid_size) + static_cast<float>(world.GetMapXOffset())),
			((itemInstance->GetY() * Globals::grid_size) + static_cast<float>(world.GetMapYOffset())));

		auto it = turrets.find(itemInstance->GetUid());

		if (it != turrets.end())
		{
			it->second->Update(itemInstance, spritesRef);
		}
	}

	MODULE_API void Delete(int uid)
	{
		turrets.erase(uid);

		Log::Print(StringConcat("Current size of the turrets map: ", turrets.size()));
	}

	MODULE_API void Destroy(const String &name)
	{
		Log::Print(StringConcat("Destroy turret { ", name, " } "));
	}
}
} // namespace TGX
