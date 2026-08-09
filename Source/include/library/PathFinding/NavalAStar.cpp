#include "NavalAStar.h"

#include <ankerl/unordered_dense.h>
#include <cmath>
#include <cstddef>
#include <memory>
#include "DataStructures/Heap.h"
#include "Logs.h"
#include "Node/Node.h"

namespace TGX
{
namespace
{
int DirectionCode(int dx, int dy)
{
	return ((dx + 1) * 3) + (dy + 1);
}

long long StateKey(int cellIndex, int dx, int dy)
{
	return (static_cast<long long>(cellIndex) * 9) + DirectionCode(dx, dy);
}

float StepCost(int fromX, int fromY, int toX, int toY)
{
	auto dx = static_cast<float>(fromX - toX);
	auto dy = static_cast<float>(fromY - toY);

	return std::sqrt((dx * dx) + (dy * dy));
}
} // namespace

void NavalAStar::Search(
	const Point &start,
	const Point &end,
	int cols, int rows,
	const Vector<Vector<int>> &grid,
	const Unique<ConfigurePath> &configure)
{
	path.clear();

	if (start.x < 0 || start.y < 0 || start.x >= cols || start.y >= rows ||
		end.x < 0 || end.y < 0 || end.x >= cols || end.y >= rows)
	{
		Log::Error("NavalAStar: start or end lies outside the grid");
		return;
	}

	float turnPenalty = 0.0f;
	if (const auto *navalConfigure = dynamic_cast<const NavalRoutesConfigurePath *>(configure.get()))
	{
		turnPenalty = navalConfigure->turnPenalty;
	}

	HeuristicType heuristicType = configure->GetHeuristicType();
	auto &heuristicFunction = heuristicFunctions[heuristicType];

	auto passableAt = [&](int x, int y) {
		if ((x == start.x && y == start.y) || (x == end.x && y == end.y))
		{
			return true;
		}

		return configure->IsTraversable(grid[y][x]);
	};

	auto limit = static_cast<size_t>(static_cast<long long>(cols) * rows * 9);

	ankerl::unordered_dense::set<long long> closed;
	ankerl::unordered_dense::map<long long, float> gScore;
	Heap<Node *> openHeap = Heap<Node *>(limit);

	Vector<std::unique_ptr<Node>> nodes;

	auto pushNode = [&](std::unique_ptr<Node> node) {
		openHeap.Add(node.get());
		nodes.emplace_back(std::move(node));
	};

	int startIndex = start.x + (start.y * cols);

	auto startNode = std::make_unique<Node>(start.x, start.y, 0.0f, 0.0f, 0.0f);
	startNode->dx = 0;
	startNode->dy = 0;

	gScore[StateKey(startIndex, 0, 0)] = 0.0f;
	pushNode(std::move(startNode));

	Vector<Point> neighbours;
	neighbours.reserve(8);

	while (true)
	{
		Node *current = openHeap.RemoveFirst();

		if (current == nullptr)
		{
			Log::Error("NavalAStar: no path to end");
			return;
		}

		int currentIndex = current->x + (current->y * cols);
		long long currentState = StateKey(currentIndex, current->dx, current->dy);

		if (closed.contains(currentState))
		{
			continue;
		}

		closed.insert(currentState);

		if (current->x == end.x && current->y == end.y)
		{
			while (current != nullptr && current->parent != nullptr)
			{
				path.emplace_back(current->x, current->y);
				current = current->parent;
			}

			return;
		}

		int n = current->y - 1;
		int s = current->y + 1;
		int e = current->x + 1;
		int w = current->x - 1;

		bool N = (n > -1) && passableAt(current->x, n);
		bool S = (s < rows) && passableAt(current->x, s);
		bool E = (e < cols) && passableAt(e, current->y);
		bool W = (w > -1) && passableAt(w, current->y);

		neighbours.clear();

		if (N)
		{
			neighbours.emplace_back(current->x, n);
		}
		if (E)
		{
			neighbours.emplace_back(e, current->y);
		}
		if (S)
		{
			neighbours.emplace_back(current->x, s);
		}
		if (W)
		{
			neighbours.emplace_back(w, current->y);
		}

		if (N)
		{
			if (E && passableAt(e, n))
			{
				neighbours.emplace_back(e, n);
			}
			if (W && passableAt(w, n))
			{
				neighbours.emplace_back(w, n);
			}
		}

		if (S)
		{
			if (E && passableAt(e, s))
			{
				neighbours.emplace_back(e, s);
			}
			if (W && passableAt(w, s))
			{
				neighbours.emplace_back(w, s);
			}
		}

		for (const auto &neighbour : neighbours)
		{
			int stepX = neighbour.x - current->x;
			int stepY = neighbour.y - current->y;

			bool turned = (current->dx != 0 || current->dy != 0) &&
						  (stepX != current->dx || stepY != current->dy);

			float newG = current->g +
						 StepCost(current->x, current->y, neighbour.x, neighbour.y) +
						 (turned ? turnPenalty : 0.0f);

			int neighbourIndex = neighbour.x + (neighbour.y * cols);
			long long neighbourState = StateKey(neighbourIndex, stepX, stepY);

			if (closed.contains(neighbourState))
			{
				continue;
			}

			auto known = gScore.find(neighbourState);

			if (known != gScore.end() && known->second <= newG)
			{
				continue;
			}

			auto newNode = std::make_unique<Node>(neighbour.x, neighbour.y);

			newNode->g = newG;
			newNode->h = heuristicFunction(
				static_cast<float>(neighbour.x),
				static_cast<float>(neighbour.y),
				static_cast<float>(end.x),
				static_cast<float>(end.y));
			newNode->f = newNode->g + newNode->h;
			newNode->parent = current;
			newNode->dx = stepX;
			newNode->dy = stepY;

			gScore[neighbourState] = newG;
			pushNode(std::move(newNode));
		}
	}
}

Vector<Point> NavalAStar::GetPath() const
{
	return path;
}
} // namespace TGX
