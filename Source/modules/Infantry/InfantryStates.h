#pragma once

#include "Flags.h"
#include "ItemInstance.h"
#include "NextStep.h"
#include "Point.h"

namespace TGX
{
class InfantryState : public ItemInstance
{
public:
	InfantryState()
	{
		cellCollisionMode = 1;
		originX = 2.0f;
		originY = 2.0f;
		priority = 1;
	}

	Vector<Point> path;
	NextStep nextStep;
	bool hasNextStep = false;
	int animationOffset = 0;
	int animationLimit = 2;
	int animationSpeedLimit = 15;
	bool attacking = false;
	int reloadTimeLeft = 0;
	ItemInstance *loadedVehicle = nullptr;

	static inline int cellCollisionMode = 1;

	static constexpr Array<float, 16> cosDirectionAngles = {
		1.f, 0.92387953f, 0.70710678f, 0.38268343f,
		6.123234e-17f, -0.38268343f, -0.70710678f, -0.92387953f,
		-1.f, -0.92387953f, -0.70710678f, -0.38268343f,
		-1.83697e-16f, 0.38268343f, 0.70710678f, 0.92387953f};

	static constexpr Array<float, 16> sinDirectionAngles = {
		0.f, 0.38268343f, 0.70710678f, 0.92387953f,
		1.f, 0.92387953f, 0.70710678f, 0.38268343f,
		1.2246468e-16f, -0.38268343f, -0.70710678f, -0.92387953f,
		-1.f, -0.92387953f, -0.70710678f, -0.38268343f};

	int GetCellCollisionMode() const override
	{
		return cellCollisionMode;
	}

	virtual int GetTurnSpeed() const = 0;
	virtual float GetSpeed() const = 0;
	virtual int GetReloadTime() const = 0;

	void AddToGrid(Vector<Vector<int>> &grid, GridTracker &gridTracker) const override
	{
		if (gridTracker.uids_grid.find(uid) != gridTracker.uids_grid.end())
		{
			return;
		}

		int mapGridHeight = static_cast<int>(grid.size());
		int mapGridWidth = static_cast<int>(grid[0].size());
		float radius = GetRadius() / 20;

		int x1 = std::max(static_cast<int>(std::floor(x - radius)), 0);
		int x2 = std::min(static_cast<int>(std::floor(x + radius)), mapGridWidth - 1);
		int y1 = std::max(static_cast<int>(std::floor(y - radius)), 0);
		int y2 = std::min(static_cast<int>(std::floor(y + radius)), mapGridHeight - 1);

		for (int gridX = x1; gridX <= x2; ++gridX)
		{
			for (int gridY = y1; gridY <= y2; ++gridY)
			{
				if (grid[gridY][gridX] < Flags::CELL_COLLISION_MODE_HARD)
				{
					String cellKey = std::to_string(gridX) + " " + std::to_string(gridY);

					if (gridTracker.cells_grid.find(cellKey) != gridTracker.cells_grid.end())
					{
						int cellStack = gridTracker.cells_grid[cellKey];
						cellStack += cellCollisionMode;
						gridTracker.cells_grid[cellKey] = cellStack;
					}
					else
					{
						gridTracker.cells_grid[cellKey] = cellCollisionMode;
					}

					int cellStack = gridTracker.cells_grid[cellKey];
					if (cellStack >= Flags::CELL_COLLISION_MODE_MEDIUM && cellStack < Flags::CELL_COLLISION_MODE_HARD)
					{
						grid[gridY][gridX] = Flags::CELL_COLLISION_MODE_MEDIUM;
					}
					else if (cellStack > Flags::CELL_COLLISION_MODE_OFF)
					{
						grid[gridY][gridX] = Flags::CELL_COLLISION_MODE_SOFT;
					}
				}
			}
		}

		gridTracker.uids_grid[uid] = {x1, y1, x2, y2};
	}

	void RemoveFromGrid(Vector<Vector<int>> &grid, GridTracker &gridTracker) const override
	{
		auto it = gridTracker.uids_grid.find(uid);
		if (it == gridTracker.uids_grid.end())
		{
			return;
		}

		int x1 = std::get<0>(it->second);
		int y1 = std::get<1>(it->second);
		int x2 = std::get<2>(it->second);
		int y2 = std::get<3>(it->second);

		for (int gridX = x1; gridX <= x2; ++gridX)
		{
			for (int gridY = y1; gridY <= y2; ++gridY)
			{
				if (grid[gridY][gridX] >= Flags::CELL_COLLISION_MODE_SOFT)
				{
					String cellKey = std::to_string(gridX) + " " + std::to_string(gridY);

					auto cellIt = gridTracker.cells_grid.find(cellKey);

					if (cellIt != gridTracker.cells_grid.end())
					{
						int cellStack = cellIt->second;
						cellStack -= cellCollisionMode;

						gridTracker.cells_grid[cellKey] = cellStack;

						if (cellStack == Flags::CELL_COLLISION_MODE_OFF)
						{
							grid[gridY][gridX] = Flags::CELL_COLLISION_MODE_OFF;
						}
						else if (cellStack < Flags::CELL_COLLISION_MODE_MEDIUM)
						{
							grid[gridY][gridX] = Flags::CELL_COLLISION_MODE_SOFT;
						}
					}
				}
			}
		}

		gridTracker.uids_grid.erase(uid);
	}
};

class SoldierState : public InfantryState
{
public:
	static constexpr float radius = 9.0f;
	static constexpr int frames = 16;
	static constexpr int directions = 8;
	static constexpr int turnSpeed = 360;
	static constexpr float speed = 150;
	static constexpr int sight = 6;
	static constexpr int reloadTime = 120;

	SoldierState()
	{
		SetDirections(directions);
		AddGroup("army");

		combat.weapon = "bullet";
		combat.enabled = true;
		combat.attackable = true;

		logistics.canExtract = false;
		logistics.loadable = true;

		hitPoints = 25.0f;
	}

	float GetRadius() const override
	{
		return radius;
	}
	int GetFrames() const override
	{
		return frames;
	}
	float GetSpeed() const override
	{
		return speed;
	}
	int GetTurnSpeed() const override
	{
		return turnSpeed;
	}
	int GetSight() const override
	{
		return sight;
	}
	int GetReloadTime() const override
	{
		return reloadTime;
	}
};

class GrenadierState : public InfantryState
{
public:
	static constexpr float radius = 9.0f;
	static constexpr int frames = 16;
	static constexpr int directions = 8;
	static constexpr int turnSpeed = 240;
	static constexpr float speed = 180;
	static constexpr int sight = 8;
	static constexpr int reloadTime = 120;

	GrenadierState()
	{
		SetDirections(directions);
		AddGroup("army");

		combat.weapon = "grenade";
		combat.enabled = true;
		combat.attackable = true;

		logistics.canExtract = false;
		logistics.loadable = true;

		hitPoints = 25.0f;
	}

	float GetRadius() const override
	{
		return radius;
	}
	int GetFrames() const override
	{
		return frames;
	}
	float GetSpeed() const override
	{
		return speed;
	}
	int GetTurnSpeed() const override
	{
		return turnSpeed;
	}
	int GetSight() const override
	{
		return sight;
	}
	int GetReloadTime() const override
	{
		return reloadTime;
	}
};

class RocketeerState : public InfantryState
{
public:
	static constexpr float radius = 9.0f;
	static constexpr int frames = 16;
	static constexpr int directions = 8;
	static constexpr int turnSpeed = 180;
	static constexpr float speed = 90;
	static constexpr int sight = 8;
	static constexpr int reloadTime = 120;

	RocketeerState()
	{
		SetDirections(directions);
		AddGroup("army");

		combat.weapon = "rocket";
		combat.enabled = true;
		combat.attackable = true;

		logistics.canExtract = false;
		logistics.loadable = true;

		hitPoints = 25.0f;
	}

	float GetRadius() const override
	{
		return radius;
	}
	int GetFrames() const override
	{
		return frames;
	}
	float GetSpeed() const override
	{
		return speed;
	}
	int GetTurnSpeed() const override
	{
		return turnSpeed;
	}
	int GetSight() const override
	{
		return sight;
	}
	int GetReloadTime() const override
	{
		return reloadTime;
	}
};
} // namespace TGX
