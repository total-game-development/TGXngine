#pragma once

#include <Common.hpp>
#include "Core.h"
#include "Flags.h"
#include "Globals.h"
#include "Logs.h"
#include "NavigationStructures.h"
#include "Orders.h"
#include "PathFinding/AStar.h"
#include "PathFinding/ConfigurePath.h"
#include "PathFinding/IPathfinding.h"
#include "PathFinding/NavalAStar.h"
#include "PathFinding/WavePropagation.h"
#include "Point.h"
#include "Traversal/Traversal.h"
#include "WorldState.h"

namespace TGX
{
class Navigation
{
public:
	static constexpr size_t MAX_NAVIGATION_TRACKER_SIZE = 10;
	static constexpr size_t MAX_DIRECTION_TRACKER_SIZE = 50;
	static constexpr size_t ISLE_OFFSET = 5;
	static constexpr size_t ISLE_ADJUSTMENT = 5;

	Navigation(const Navigation &) = delete;
	Navigation &operator=(const Navigation &) = delete;
	Navigation(Navigation &&) = delete;
	Navigation &operator=(Navigation &&) = delete;

	static Navigation &GetInstance()
	{
		static Navigation instance;
		return instance;
	}

	static void Init()
	{
		static bool isInitialized = false;
		if (isInitialized)
		{
			Log::Print("Navigation already initialized. Skipping.");
			return;
		}

		Log::Warning("Navigation Init");

		auto &instance = GetInstance();
		instance.pathfindingAlgorithms[PathfindingAlgorithm::AStar] = std::make_unique<AStar>();
		instance.pathfindingAlgorithms[PathfindingAlgorithm::Naval] = std::make_unique<NavalAStar>();
		instance.pathfindingAlgorithms[PathfindingAlgorithm::Wave] = std::make_unique<WavePropagation>();

		isInitialized = true;
	}

	static void AddToGroup(int id, int x, int y, int /*uid*/, Orders &orders)
	{
		auto &instance = GetInstance();
		auto it = instance.navigationGroups.find(id);
		if (it != instance.navigationGroups.end())
		{
			it->second.n++;
			return;
		}

		WorldState &world = WorldState::GetInstance();

		const float adjustedX = static_cast<float>(x) + world.GetMapXOffset();
		const float adjustedY = static_cast<float>(y) + world.GetMapYOffset();

		const int MOVE_TO_TOLERANCE = Globals::NAVIGATION_GROUP_MOVE_TO_OFFSET * Globals::grid_size;

		for (auto &[existingId, group] : instance.navigationGroups)
		{
			if (abs(group.x - adjustedX) < MOVE_TO_TOLERANCE &&
				abs(group.y - adjustedY) < MOVE_TO_TOLERANCE)
			{
				group.n++;
				orders.id = existingId;
				return;
			}
		}

		int finalX = static_cast<int>(adjustedX);
		int finalY = static_cast<int>(adjustedY);

		instance.navigationGroups[id] = NavigationGroup{.x = finalX, .y = finalY, .n = 1};
		instance.navigationGroupTracker.push_back(id);

		if (instance.navigationGroupTracker.size() > MAX_NAVIGATION_TRACKER_SIZE)
		{
			instance.navigationGroups.erase(instance.navigationGroupTracker.front());
			instance.navigationGroupTracker.erase(instance.navigationGroupTracker.begin());
		}
	}

	static bool CheckForGroupIds(int id)
	{
		return GetInstance().navigationGroups.contains(id);
	}

	static void RemoveFromGroup(int id)
	{
		auto &instance = GetInstance();
		auto it = instance.navigationGroups.find(id);
		if (it != instance.navigationGroups.end())
		{
			NavigationGroup &navigationGroup = it->second;
			navigationGroup.n--;

			if (navigationGroup.n == 0)
			{
				WorldState &world = WorldState::GetInstance();

				for (auto &item : world.items)
				{
					if (item->GetOrders()->id == id)
					{
						continue;
					}

					item->SetOrders(Orders::Order::DetourTo);
				}

				instance.navigationGroups.erase(id);

				instance.navigationGroupTracker.erase(
					std::remove(instance.navigationGroupTracker.begin(), instance.navigationGroupTracker.end(), id), // NOLINT(modernize-use-ranges)
					instance.navigationGroupTracker.end());
			}
		}
	}

	static int GetGlobalDirection(int id, int direction)
	{
		auto &instance = GetInstance();
		auto it = instance.directionGroups.find(id);
		if (it != instance.directionGroups.end())
		{
			return it->second;
		}

		instance.directionGroups[id] = direction;
		instance.directionGroupTracker.push(id);

		if (instance.directionGroups.size() > MAX_DIRECTION_TRACKER_SIZE)
		{
			int oldestId = instance.directionGroupTracker.front();
			instance.directionGroupTracker.pop();
			instance.directionGroups.erase(oldestId);
		}

		return direction;
	}

	static void DeleteAllMarkers(Vector<Vector<int>> &terrainGrid)
	{
		WorldState &world = WorldState::GetInstance();

		if (world.terrainMarkers.empty())
		{
			return;
		}

		for (const Point &marker : world.terrainMarkers)
		{
			terrainGrid[marker.y][marker.x] = Flags::CELL_COLLISION_MODE_OFF;
		}

		world.terrainMarkers.clear();
	}

	static void CreateWayPoints()
	{
		Log::Info("CreateWayPoints");
		WorldState &world = WorldState::GetInstance();

		size_t rows = world.currentIsleMapPassableGrid.size() / ISLE_OFFSET;
		size_t cols = world.currentIsleMapPassableGrid[0].size() / ISLE_OFFSET;

		world.wayPoints.resize(rows);

		for (size_t i = 0; i < rows; i++)
		{
			world.wayPoints[i].resize(cols);
		}

		for (size_t i = 1; i < rows; i++)
		{
			for (size_t j = 1; j < cols; j++)
			{
				if (world.currentIsleMapPassableGrid[i * ISLE_OFFSET][j * ISLE_OFFSET] == Flags::CELL_COLLISION_MODE_MEDIUM ||
					world.currentIsleMapPassableGrid[i * ISLE_OFFSET][j * ISLE_OFFSET] == Flags::CELL_COLLISION_MODE_SOFT)
				{
					world.wayPoints[i - 1][j - 1] = -1;
					continue;
				}

				WayPoint point;
				point.x = static_cast<int>(j * ISLE_OFFSET);
				point.y = static_cast<int>(i * ISLE_OFFSET);
				point.locked = Flags::LAYER_UNLOCKED;

				world.wayPoints[i - 1][j - 1] = Flags::LAYER_UNLOCKED;
			}
		}

		for (size_t i = 1; i < rows; i++)
		{
			for (size_t j = 1; j < cols; j++)
			{
				if (world.wayPoints[i - 1][j - 1] == -1)
				{
					continue;
				}

				const int offset = static_cast<int>(ISLE_OFFSET);
				const int adjustment = static_cast<int>(ISLE_ADJUSTMENT);
				int L = static_cast<int>(j * offset) - adjustment;
				int R = static_cast<int>(j * offset) + adjustment;
				int U = static_cast<int>(i * offset) - adjustment;
				int D = static_cast<int>(i * offset) + adjustment;
				bool removeWayPoint = false;

				for (int x = L; x < R; x++)
				{
					if (world.currentIsleMapPassableGrid[i * ISLE_OFFSET][x] == Flags::CELL_COLLISION_MODE_MEDIUM ||
						world.currentIsleMapPassableGrid[i * ISLE_OFFSET][x] == Flags::CELL_COLLISION_MODE_SOFT)
					{
						removeWayPoint = true;
						break;
					}
				}

				for (int y = U; y < D; y++)
				{
					if (world.currentIsleMapPassableGrid[y][j * ISLE_OFFSET] == Flags::CELL_COLLISION_MODE_MEDIUM ||
						world.currentIsleMapPassableGrid[y][j * ISLE_OFFSET] == Flags::CELL_COLLISION_MODE_SOFT)
					{
						removeWayPoint = true;
						break;
					}
				}

				L = static_cast<int>(j * offset) - (adjustment * 2);
				R = static_cast<int>(j * offset) + (adjustment * 2);
				U = static_cast<int>(i * offset) - (adjustment * 2);
				D = static_cast<int>(i * offset) + (adjustment * 2);

				int numberOfHorizontalNeighbours = 0;
				int numberOfVerticalNeighbours = 0;

				if (L >= 0 && world.currentIsleMapPassableGrid[i * ISLE_OFFSET][L] < Flags::CELL_COLLISION_MODE_SOFT)
				{
					numberOfHorizontalNeighbours++;
				}

				if (R < static_cast<int>(world.currentIsleMapPassableGrid[0].size()) && world.currentIsleMapPassableGrid[i * ISLE_OFFSET][R] < Flags::CELL_COLLISION_MODE_SOFT)
				{
					numberOfHorizontalNeighbours++;
				}

				if (U >= 0 && world.currentIsleMapPassableGrid[U][j * ISLE_OFFSET] < Flags::CELL_COLLISION_MODE_SOFT)
				{
					numberOfVerticalNeighbours++;
				}

				if (D < static_cast<int>(world.currentIsleMapPassableGrid.size()) && world.currentIsleMapPassableGrid[D][j * ISLE_OFFSET] < Flags::CELL_COLLISION_MODE_SOFT)
				{
					numberOfVerticalNeighbours++;
				}

				if (numberOfHorizontalNeighbours != 2 || numberOfVerticalNeighbours != 0 ||
					(numberOfHorizontalNeighbours == 0 && numberOfVerticalNeighbours == 2))
				{
					if (removeWayPoint)
					{
						world.wayPoints[i - 1][j - 1] = -1;
					}
				}
			}
		}

		int numberOfWayPoints = 0;
		String wayPointsOutput;

		for (size_t i = 1; i < world.wayPoints.size(); i++)
		{
			for (size_t j = 1; j < world.wayPoints[0].size(); j++)
			{
				if (world.wayPoints[i - 1][j - 1] == Flags::LAYER_UNLOCKED)
				{
					numberOfWayPoints++;
					wayPointsOutput += "*";
				}
				else if (world.wayPoints[i - 1][j - 1] == -1)
				{
					wayPointsOutput += "#";
				}
			}
			wayPointsOutput += "\n";
		}

		Log::Info("Number of Waypoints: " + std::to_string(numberOfWayPoints));
	}

	static void LockWayPoints(float &shipX, float &shipY, int x1, int y1, int x2, int y2, int locked)
	{
		WorldState &world = WorldState::GetInstance();

		int indexShipX = static_cast<int>(std::round(shipX / 5)) - 1;
		int indexShipY = static_cast<int>(std::round(shipY / 5)) - 1;

		if (world.wayPoints[indexShipY][indexShipX])
		{
			world.wayPoints[indexShipY][indexShipX] = locked;
		}

		indexShipX = std::max(0, (indexShipX + x1));
		indexShipY = std::max(0, (indexShipY + y1));

		if (world.wayPoints[indexShipY][indexShipX])
		{
			world.wayPoints[indexShipY][indexShipX] = locked;
		}

		indexShipX -= x1;
		indexShipY -= y1;

		indexShipX = std::min(static_cast<int>(world.wayPoints.size() - 1), (indexShipX + x2));
		indexShipY = std::min(static_cast<int>(world.wayPoints[0].size() - 1), (indexShipY + y2));

		if (world.wayPoints[indexShipY][indexShipX])
		{
			world.wayPoints[indexShipY][indexShipX] = locked;
		}
	}

	static int CalcPathLength(ItemInstance &item, const Vector<Point> &path, const Vector<WayPoint> *wayPoints)
	{
		int pathLength = 0;
		int distanceFromDestinationSquared = 0;

		if (!path.empty())
		{
			distanceFromDestinationSquared =
				static_cast<int>(
					std::pow(item.GetX() - static_cast<float>(path[0].x), 2) +
					std::pow(item.GetY() - static_cast<float>(path[0].y), 2));
		}

		if (wayPoints)
		{
			pathLength += static_cast<int>(wayPoints->size() * 1000000);
			pathLength += static_cast<int>(path.size() * 1000);
			pathLength += distanceFromDestinationSquared;
		}
		else
		{
			pathLength += static_cast<int>(path.size() * 1000);
			pathLength += distanceFromDestinationSquared;
		}

		return pathLength;
	}

	static Vector<Vector<int>> &CopyGrid(const Vector<Vector<int>> &currentTerrainMapPassableGrid)
	{
		auto &instance = GetInstance();
		if (!instance.currentGrid.empty())
		{
			return instance.currentGrid;
		}

		instance.currentGrid.resize(currentTerrainMapPassableGrid.size());

		for (size_t i = 0; i < currentTerrainMapPassableGrid.size(); ++i)
		{
			instance.currentGrid[i].resize(currentTerrainMapPassableGrid[i].size());
			std::copy(currentTerrainMapPassableGrid[i].begin(), currentTerrainMapPassableGrid[i].end(), instance.currentGrid[i].begin());
		}

		return instance.currentGrid;
	}

	static Vector<Vector<int>> &CopyIsleGrid(const Vector<Vector<int>> &currentIsleMapPassableGrid)
	{
		auto &instance = GetInstance();
		if (!instance.currentIsleGrid.empty())
		{
			return instance.currentIsleGrid;
		}

		instance.currentIsleGrid.resize(currentIsleMapPassableGrid.size());

		for (size_t i = 0; i < currentIsleMapPassableGrid.size(); ++i)
		{
			instance.currentIsleGrid[i].resize(currentIsleMapPassableGrid[i].size());
			std::copy(currentIsleMapPassableGrid[i].begin(), currentIsleMapPassableGrid[i].end(), instance.currentIsleGrid[i].begin());
		}

		return instance.currentIsleGrid;
	}

	static Vector<Point> GetPath(
		Vector<Vector<int>> grid,
		Point start,
		Point end,
		int cellMode,
		PathfindingAlgorithm pathfinding = PathfindingAlgorithm::AStar,
		HeuristicType heuristicType = HeuristicType::Euclidean,
		TraversalType traversal = TraversalType::Normal)
	{
		Unique<ConfigurePath> config;

		switch (pathfinding)
		{
			case (PathfindingAlgorithm::AStar):
				{
					auto astarConfig = std::make_unique<AStarConfigurePath>();
					astarConfig->heuristicType = heuristicType;
					astarConfig->traversalLogic = GetTraversalPredicate(traversal);

					if (traversal == TraversalType::Soft)
					{
						astarConfig->collisionMode = CellCollisionMode::Soft;
					}
					else
					{
						astarConfig->collisionMode = CellCollisionMode::Off;
					}

					astarConfig->codeMode = cellMode;
					config = std::move(astarConfig);
					break;
				}
			case (PathfindingAlgorithm::Naval):
				{
					auto navalConfig = std::make_unique<NavalRoutesConfigurePath>();
					navalConfig->codeMode = cellMode;
					config = std::move(navalConfig);
					break;
				}
			case (PathfindingAlgorithm::Wave):
				{
					auto waveConfig = std::make_unique<WaveConfigurePath>();
					waveConfig->codeMode = cellMode;
					config = std::move(waveConfig);
					break;
				}
			default:
				{
					Log::Print("Unknown path finding algorithm");
					return {};
				}
		}

		auto &instance = GetInstance();

		auto algorithm = instance.pathfindingAlgorithms.find(pathfinding);

		if (algorithm == instance.pathfindingAlgorithms.end() || !algorithm->second)
		{
			Log::Error("Path finding algorithm requested but not registered in Navigation::Init");
			return {};
		}

		algorithm->second->Search(
			start,
			end,
			static_cast<int>(grid[0].size()),
			static_cast<int>(grid.size()),
			grid,
			config);

		return algorithm->second->GetPath();
	}

	static void Clear()
	{
		auto &instance = GetInstance();
		instance.navigationGroupTracker.clear();
		instance.navigationGroups.clear();
		instance.directionGroups.clear();
		instance.terrainMarkers.clear();
		instance.currentGrid.clear();
		instance.currentIsleGrid.clear();

		while (!instance.directionGroupTracker.empty())
		{
			instance.directionGroupTracker.pop();
		}
	}

private:
	Navigation() = default;
	~Navigation() = default;

	// Instance Members
	Vector<int> navigationGroupTracker;
	Queue<int> directionGroupTracker;
	Map<int, NavigationGroup> navigationGroups;
	Map<int, int> directionGroups;
	Set<Point> terrainMarkers;
	Vector<Vector<int>> currentGrid;
	Vector<Vector<int>> currentIsleGrid;

	Map<PathfindingAlgorithm, Unique<IPathfinding>> pathfindingAlgorithms;
	PathfindingAlgorithm pathfindingAlgorithm = PathfindingAlgorithm::AStar;
};
} // namespace TGX
