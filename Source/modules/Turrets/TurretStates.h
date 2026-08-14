#pragma once

#include <cmath>
#include "Core.h"
#include "ItemInstance.h"

namespace TGX
{
class TurretState : public ItemInstance
{
public:
	Vector<Vector<int>> passableGrid;
	int baseWidth = 0;
	int baseHeight = 0;
	int reloadTimeLeft = 0;

	bool canTargetLand = true;
	bool canTargetAir = false;

	static inline int cellCollisionMode = 1000;

	int GetCellCollisionMode() const override
	{
		return cellCollisionMode;
	}

	float GetCenterX() const override
	{
		if (passableGrid.empty() || passableGrid[0].empty()) { return GetX(); }

		return GetX() + (static_cast<float>(passableGrid[0].size()) / 2.0f);
	}

	float GetCenterY() const override
	{
		if (passableGrid.empty()) { return GetY(); }

		return GetY() + (static_cast<float>(passableGrid.size()) / 2.0f);
	}

	void AddToGrid(Vector<Vector<int>> & /*grid*/, GridTracker & /*gridTracker*/) const override
	{
	}

	void RemoveFromGrid(Vector<Vector<int>> & /*grid*/, GridTracker & /*gridTracker*/) const override
	{
	}

	float GetRadius() const override = 0;
	int GetFrames() const override = 0;
	int GetSight() const override = 0;
	virtual int GetReloadTime() const = 0;
	virtual int GetPowerUsage() const = 0;
};

// -------------------------------------------------------------------------
// Laser Tower
// -------------------------------------------------------------------------
class LaserTowerState : public TurretState
{
public:
	static constexpr float radius = 20.0f;
	static constexpr int frames = 1;
	static constexpr int directions = 8;
	static constexpr int sight = 14;
	static constexpr int reloadTime = 60;
	static constexpr int powerUsage = 200;

	LaserTowerState()
	{
		passableGrid = Vector<Vector<int>>(8, Vector<int>(2, 1));
		baseWidth = 50;
		baseHeight = 80;

		SetDirections(directions);
		AddGroup("turrets");

		combat.enabled = true;
		combat.attackable = true;
		combat.weapon = "laser";

		classification.armoured = true;

		canTargetLand = true;
		canTargetAir = false;

		hitPoints = 300.0f;
	}

	float GetRadius() const override
	{
		return radius;
	}
	int GetFrames() const override
	{
		return frames;
	}
	int GetSight() const override
	{
		return sight;
	}
	int GetReloadTime() const override
	{
		return reloadTime;
	}
	int GetPowerUsage() const override
	{
		return powerUsage;
	}
};

// -------------------------------------------------------------------------
// Missile Turret
// -------------------------------------------------------------------------
class MissileTurretState : public TurretState
{
public:
	static constexpr float radius = 20.0f;
	static constexpr int frames = 1;
	static constexpr int directions = 8;
	static constexpr int sight = 20;
	static constexpr int reloadTime = 60;
	static constexpr int powerUsage = 50;

	MissileTurretState()
	{
		passableGrid = Vector<Vector<int>>(4, Vector<int>(4, 1));
		baseWidth = 20;
		baseHeight = 80;

		SetDirections(directions);
		AddGroup("turrets");

		combat.enabled = true;
		combat.attackable = true;
		combat.weapon = "missile";

		classification.armoured = true;

		canTargetLand = false;
		canTargetAir = true;

		hitPoints = 500.0f;
	}

	float GetRadius() const override
	{
		return radius;
	}
	int GetFrames() const override
	{
		return frames;
	}
	int GetSight() const override
	{
		return sight;
	}
	int GetReloadTime() const override
	{
		return reloadTime;
	}
	int GetPowerUsage() const override
	{
		return powerUsage;
	}
};
} // namespace TGX
