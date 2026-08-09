#include "AStar.h"
#include <cstddef>
#include <memory>
#include "DataStructures/Heap.h"
#include "Logs.h"
#include "Node/Node.h"
#include "StringUtils.hpp"

namespace TGX
{
void AStar::Test()
{
}

void AStar::Search(
	const Point &start,
	const Point &end,
	int cols, int rows,
	const Vector<Vector<int>> &grid,
	const Unique<ConfigurePath> &configure)
{
	if (grid[end.y][end.x] == 1)
	{
		Log::Error("AStar NO PATH TO END");
		return;
	}

	path.clear();

	HeuristicType heuristicType = configure->GetHeuristicType();
	auto &heuristicFunction = heuristicFunctions[heuristicType];

	size_t limit = (static_cast<long>(cols) * rows);
	size_t neighboursIndex;

	Vector<Node> neighbours;
	neighbours.reserve(neighbour_limit);

	for (size_t i = 0; i < static_cast<size_t>(neighbour_limit); i++)
	{
		neighbours.emplace_back();
	}

	ankerl::unordered_dense::set<int> closed;
	ankerl::unordered_dense::set<int> openSet;
	Heap<Node *> openHeap = Heap<Node *>(limit);

	ankerl::unordered_dense::map<int, std::unique_ptr<Node>> nodeMap;

	Node *current;

	int startIndex = CoordinateToIndex(start.x, start.y, cols);
	openSet.insert(startIndex);

	auto startNode = std::make_unique<Node>(
		static_cast<int>(start.x),
		static_cast<int>(start.y),
		0.0f,
		0.0f,
		static_cast<float>(start.x + (start.y * cols)));

	openHeap.Add(startNode.get());
	nodeMap[startIndex] = std::move(startNode);

	while (true)
	{
		current = openHeap.RemoveFirst();

		if (current == nullptr)
		{
			Log::Error("Exit early");
			break;
		}

		int currentIndex = current->x + (current->y * cols);
		auto it = closed.find(currentIndex);

		if (it == closed.end())
		{
			closed.insert(currentIndex);
		}
		else
		{
			continue;
		}

		if (aStar_counter == aStar_limit)
		{
			break;
		}

		if (current->x != end.x || current->y != end.y)
		{
			int n = current->y - 1;
			int s = current->y + 1;
			int e = current->x + 1;
			int w = current->x - 1;

			bool N = (n > -1) && configure->IsTraversable(grid[n][current->x]);
			bool S = (s < rows) && configure->IsTraversable(grid[s][current->x]);
			bool E = (e < cols) && configure->IsTraversable(grid[current->y][e]);
			bool W = (w > -1) && configure->IsTraversable(grid[current->y][w]);

			neighboursIndex = 0;

			if (N)
			{
				neighbours[neighboursIndex].x = current->x;
				neighbours[neighboursIndex++].y = n;
			}
			if (E)
			{
				neighbours[neighboursIndex].x = e;
				neighbours[neighboursIndex++].y = current->y;
			}
			if (S)
			{
				neighbours[neighboursIndex].x = current->x;
				neighbours[neighboursIndex++].y = s;
			}
			if (W)
			{
				neighbours[neighboursIndex].x = w;
				neighbours[neighboursIndex++].y = current->y;
			}

			if (N)
			{
				if (E && configure->IsTraversable(grid[n][e]))
				{
					neighbours[neighboursIndex].x = e;
					neighbours[neighboursIndex++].y = n;
				}
				if (W && configure->IsTraversable(grid[n][w]))
				{
					neighbours[neighboursIndex].x = w;
					neighbours[neighboursIndex++].y = n;
				}
			}

			if (S)
			{
				if (E && configure->IsTraversable(grid[s][e]))
				{
					neighbours[neighboursIndex].x = e;
					neighbours[neighboursIndex++].y = s;
				}
				if (W && configure->IsTraversable(grid[s][w]))
				{
					neighbours[neighboursIndex].x = w;
					neighbours[neighboursIndex++].y = s;
				}
			}

			for (size_t i = 0; i < neighboursIndex; i++)
			{
				int neighborIndex = neighbours[i].x + (neighbours[i].y * cols);
				if (closed.contains(neighborIndex))
				{
					continue;
				}

				float newDistance = current->g + heuristicFunction(static_cast<float>(current->x), static_cast<float>(current->y), static_cast<float>(neighbours[i].x), static_cast<float>(neighbours[i].y));

				bool neighbourInOpen = openSet.contains(neighborIndex);

				if (newDistance < neighbours[i].g || !neighbourInOpen)
				{
					auto newNode = std::make_unique<Node>(
						static_cast<int>(neighbours[i].x),
						static_cast<int>(neighbours[i].y));

					newNode->g = newDistance;
					newNode->h = heuristicFunction(static_cast<float>(newNode->x), static_cast<float>(newNode->y), static_cast<float>(end.x), static_cast<float>(end.y));
					newNode->f = newNode->g + newNode->h;
					newNode->parent = current;

					openHeap.Add(newNode.get());
					openSet.insert(neighborIndex);

					nodeMap[neighborIndex] = std::move(newNode);
				}
			}
		}
		else
		{
			while (current != nullptr && current->parent != nullptr)
			{
				path.emplace_back(current->x, current->y);
				current = current->parent;
			}

			break;
		}
	}
}

Vector<Point> AStar::GetPath() const
{
	return path;
}
} // namespace TGX
