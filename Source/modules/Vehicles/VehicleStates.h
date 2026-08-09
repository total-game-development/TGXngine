#pragma once

#include "Flags.h"
#include "ItemInstance.h"
#include "Logs.h"
#include "NextStep.h"
#include "Point.h"

namespace TGX
{
class VehicleState : public ItemInstance
{
public:
	VehicleState()
	{
		originX = 2.0f;
		originY = 2.0f;
		priority = 2;
	}

	~VehicleState() override = default;

	Vector<Point> path;
	NextStep nextStep;
	bool hasNextStep = false;

	int animationLimit = 2;
	int animationSpeedLimit = 15;
	size_t accelerationIndex = 0;
	int velocityThreshold = 0;
	int reloadTimeLeft = 0;

	Array<float, 16> body{};
	Array<float, 16> skin{};
	Array<float, 16> vision{};
	Array<float, 16> bumper{};

	static constexpr int cellCollisionMode = 100; // Medium Collision
	int GetCellCollisionMode() const override
	{
		return cellCollisionMode;
	}

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

	static constexpr Array<float, 33> accelerationFactor = {
		1.f, 0.99f, 0.98f, 0.97f, 0.96f, 0.95f, 0.93f, 0.91f, 0.89f, 0.85f,
		0.82f, 0.8f, 0.87f, 0.85f, 0.8f, 0.75f, 0.7f, 0.65f, 0.6f, 0.55f,
		0.5f, 0.48f, 0.47f, 0.46f, 0.45f, 0.44f, 0.43f, 0.42f, 0.41f, 0.3f,
		0.1f, 0.01f, 0.f};

	float GetRadius() const override = 0;
	int GetFrames() const override = 0;
	virtual int GetTurnSpeed() const = 0;
	virtual float GetSpeed() const = 0;
	virtual int GetReloadTime() const = 0;

	void AddToGrid(Vector<Vector<int>> &grid, GridTracker &gridTracker) const override
	{
		if (gridTracker.uids_grid.find(uid) != gridTracker.uids_grid.end())
		{
			Log::Debug("AddToGrid return early");
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
					std::string cellKey = std::to_string(gridX) + " " + std::to_string(gridY);

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
		Log::Debug(static_cast<int>(gridTracker.uids_grid.size()));
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

	void AddTacticalGrid(int inUid, int pathIndex, Vector<Point> inPath, int radius, Vector<Vector<int>> &grid, GridTracker &gridTracker) const
	{
		int mapGridHeight = static_cast<int>(grid.size());
		int mapGridWidth = static_cast<int>(grid[0].size());

		int x1 = std::max(static_cast<int>(std::floor(inPath[pathIndex].x - radius)), 0);
		int x2 = std::min(static_cast<int>(std::floor(inPath[pathIndex].x + radius)), mapGridWidth - 1);
		int y1 = std::max(static_cast<int>(std::floor(inPath[pathIndex].y - radius)), 0);
		int y2 = std::min(static_cast<int>(std::floor(inPath[pathIndex].y + radius)), mapGridHeight - 1);

		for (int gridX = x1; gridX <= x2; ++gridX)
		{
			for (int gridY = y1; gridY <= y2; ++gridY)
			{
				if (grid[gridY][gridX] < Flags::CELL_COLLISION_MODE_HARD)
				{
					std::string cellKey = std::to_string(gridX) + " " + std::to_string(gridY);

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

		gridTracker.uids_grid[inUid] = {x1, y1, x2, y2};
	}
};

// -------------------------------------------------------------------------
// Prospector
// -------------------------------------------------------------------------
class ProspectorState : public VehicleState
{
public:
	static constexpr float radius = 22.0f; // Medium Collision
	static constexpr int frames = 16;
	static constexpr int directions = 16;
	static constexpr int turnSpeed = 240;
	static constexpr float speed = 360;
	static constexpr int sight = 3;
	static constexpr int reloadTime = 0;

	ProspectorState()
	{
		SetDirections(directions);
		AddGroup("army");
		AddGroup("vehicles");

		combat.weapon = "bullet";
		combat.attackable = true;

		logistics.canExtract = true;

		hitPoints = 50.0f;
	}

	float GetRadius() const override
	{
		return radius;
	}
	int GetFrames() const override
	{
		return frames;
	}
	int GetTurnSpeed() const override
	{
		return turnSpeed;
	}
	float GetSpeed() const override
	{
		return speed;
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

// -------------------------------------------------------------------------
// Truck
// -------------------------------------------------------------------------
class TruckState : public VehicleState
{
public:
	static constexpr float radius = 22.0f; // Medium Collision
	static constexpr int frames = 16;
	static constexpr int directions = 16;
	static constexpr int turnSpeed = 240;
	static constexpr float speed = 540;
	static constexpr int sight = 8;
	static constexpr int reloadTime = 8;

	TruckState()
	{
		SetDirections(directions);
		AddGroup("army");
		AddGroup("vehicles");

		combat.weapon = "bullet";
		combat.enabled = true;
		combat.attackable = true;

		// The web version calls this unit a jeep; the engine renamed it.
		hitPoints = 150.0f;
	}

	float GetRadius() const override
	{
		return radius;
	}
	int GetFrames() const override
	{
		return frames;
	}
	int GetTurnSpeed() const override
	{
		return turnSpeed;
	}
	float GetSpeed() const override
	{
		return speed;
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

class TankState : public VehicleState
{
public:
	static constexpr float radius = 22.0f; // Medium Collision
	static constexpr int frames = 16;
	static constexpr int directions = 16;
	static constexpr int turnSpeed = 240;
	static constexpr float speed = 360;
	static constexpr int sight = 12;
	static constexpr int reloadTime = 180;

	TankState()
	{
		SetDirections(directions);
		AddGroup("army");
		AddGroup("vehicles");

		combat.enabled = true;
		combat.weapon = "shell";
		combat.attackable = true;

		hitPoints = 1500.0f;
	}

	float GetRadius() const override
	{
		return radius;
	}
	int GetFrames() const override
	{
		return frames;
	}
	int GetTurnSpeed() const override
	{
		return turnSpeed;
	}
	float GetSpeed() const override
	{
		return speed;
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

class RocketTruckState : public VehicleState
{
public:
	static constexpr float radius = 22.0f; // Medium Collision
	static constexpr int frames = 16;
	static constexpr int directions = 16;
	static constexpr int turnSpeed = 180;
	static constexpr float speed = 360;
	static constexpr int sight = 12;
	static constexpr int reloadTime = 240;

	RocketTruckState()
	{
		SetDirections(directions);
		AddGroup("army");
		AddGroup("vehicles");

		combat.weapon = "rocket";
		combat.enabled = true;
		combat.attackable = true;

		hitPoints = 100.0f;
	}

	float GetRadius() const override
	{
		return radius;
	}
	int GetFrames() const override
	{
		return frames;
	}
	int GetTurnSpeed() const override
	{
		return turnSpeed;
	}
	float GetSpeed() const override
	{
		return speed;
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

class APCState : public VehicleState
{
public:
	static constexpr float radius = 22.0f; // Medium Collision
	static constexpr int frames = 16;
	static constexpr int directions = 16;
	static constexpr int turnSpeed = 240;
	static constexpr float speed = 480;
	static constexpr int sight = 10;
	static constexpr int reloadTime = 120;

	APCState()
	{
		SetDirections(directions);
		AddGroup("army");
		AddGroup("vehicles");

		logistics.canLoad = true;

		combat.attackable = true;

		hitPoints = 200.0f;

		SetExtra(ItemInstance::ItemProperty::LoadThreshold, 64);
		SetExtra(ItemInstance::ItemProperty::LoadCapacity, 5);
		SetExtra(ItemInstance::ItemProperty::LoadedUIDs, Vector<int>{});
	}

	float GetRadius() const override
	{
		return radius;
	}
	int GetFrames() const override
	{
		return frames;
	}
	int GetTurnSpeed() const override
	{
		return turnSpeed;
	}
	float GetSpeed() const override
	{
		return speed;
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
