#include "Rules.h"

#include <set>
#include "Flags.h"
#include "ItemInstance.h"
#include "Logs.h"
#include "Physics.h"
#include "WorldState.h"

namespace TGX::Rules
{
namespace
{
constexpr int REPORTED = 20;

struct Cell
{
	int x = 0;
	int y = 0;
};

struct Body
{
	int uid = 0;
	int mode = 0;
	String name;
};

String Key(int x, int y)
{
	return std::to_string(x) + " " + std::to_string(y);
}

bool Parse(const String &key, Cell &cell)
{
	const std::size_t gap = key.find(' ');

	if (gap == String::npos)
	{
		return false;
	}

	cell.x = std::atoi(key.substr(0, gap).c_str());
	cell.y = std::atoi(key.substr(gap + 1).c_str());

	return true;
}

int Mark(const Vector<Vector<int>> &grid, const Cell &cell)
{
	if (cell.y < 0 || cell.y >= static_cast<int>(grid.size()))
	{
		return -1;
	}

	if (cell.x < 0 || cell.x >= static_cast<int>(grid[cell.y].size()))
	{
		return -1;
	}

	return grid[cell.y][cell.x];
}

String Named(const Body &body)
{
	return body.name + "#" + std::to_string(body.uid);
}
} // namespace

Vector<Violation> Check()
{
	WorldState &world = WorldState::GetInstance();
	const GridTracker &tracker = Physics::GetInstance().GetGridTracker();
	const Vector<Vector<int>> &grid = world.currentTerrainMapPassableGrid;

	Vector<Violation> found;

	Map<int, const ItemInstance *> live;

	for (const auto &item : world.items)
	{
		if (item && item->GetLife() > 0.0f)
		{
			live[item->GetUid()] = item.get();
		}
	}

	// A cell carries what the bodies standing on it are worth, plus what the
	// bookings over it are worth: a unit reserves the cell it means to stop on
	// and hands it back once its body is down, so the two are counted the same
	// way and only both together account for the stack.
	Map<String, Vector<Body>> standing;
	Map<String, int> expected;

	for (const auto &[uid, box] : tracker.uids_grid)
	{
		const auto carried = live.find(uid);

		if (carried == live.end())
		{
			found.push_back({"body",
				"uid " + std::to_string(uid) + " holds " + Key(std::get<0>(box), std::get<1>(box)) + " to " + Key(std::get<2>(box), std::get<3>(box)) + ", but nothing alive carries that uid"});

			continue;
		}

		const Body body{uid, carried->second->GetCellCollisionMode(), carried->second->GetName()};

		for (int x = std::get<0>(box); x <= std::get<2>(box); x++)
		{
			for (int y = std::get<1>(box); y <= std::get<3>(box); y++)
			{
				const int mark = Mark(grid, {x, y});

				// A cell hard when the body was placed took none of it, so it is
				// not the grid's to account for and nothing stands on it.
				if (mark < 0 || mark >= Flags::CELL_COLLISION_MODE_HARD)
				{
					continue;
				}

				standing[Key(x, y)].push_back(body);
				expected[Key(x, y)] += body.mode;
			}
		}
	}

	for (const auto &[uid, reservation] : tracker.tactical_uids_grid)
	{
		if (!live.contains(uid))
		{
			found.push_back({"booking",
				"uid " + std::to_string(uid) + " has booked " + Key(reservation.x1, reservation.y1) + " to " + Key(reservation.x2, reservation.y2) + ", but nothing alive carries that uid"});

			continue;
		}

		for (int x = reservation.x1; x <= reservation.x2; x++)
		{
			for (int y = reservation.y1; y <= reservation.y2; y++)
			{
				const int mark = Mark(grid, {x, y});

				if (mark < 0 || mark >= Flags::CELL_COLLISION_MODE_HARD)
				{
					continue;
				}

				expected[Key(x, y)] += reservation.cellMode;
			}
		}
	}

	// Two bodies at rest on one cell. Infantry may share ground; anything that
	// takes a whole cell may not, whether with another of its own or with the
	// infantry standing where it came to a stop.
	std::set<Pair<int, int>> reported;

	for (const auto &[key, bodies] : standing)
	{
		if (bodies.size() < 2)
		{
			continue;
		}

		for (std::size_t first = 0; first < bodies.size(); first++)
		{
			for (std::size_t second = first + 1; second < bodies.size(); second++)
			{
				if (bodies[first].mode < Flags::CELL_COLLISION_MODE_MEDIUM &&
					bodies[second].mode < Flags::CELL_COLLISION_MODE_MEDIUM)
				{
					continue;
				}

				const Pair<int, int> pair{
					std::min(bodies[first].uid, bodies[second].uid),
					std::max(bodies[first].uid, bodies[second].uid)};

				if (reported.contains(pair))
				{
					continue;
				}

				reported.insert(pair);

				found.push_back({"overlap",
					"cell " + key + " has " + Named(bodies[first]) + " and " + Named(bodies[second]) + " stopped on it"});
			}
		}
	}

	for (const auto &[key, value] : expected)
	{
		const auto held = tracker.cells_grid.find(key);
		const int actual = held == tracker.cells_grid.end() ? 0 : held->second;

		if (actual != value)
		{
			found.push_back({"stack",
				"cell " + key + " holds " + std::to_string(actual) + " but its bodies and bookings come to " + std::to_string(value)});
		}
	}

	for (const auto &[key, value] : tracker.cells_grid)
	{
		Cell cell;

		if (!Parse(key, cell))
		{
			found.push_back({"stack", "cell key " + key + " is not a cell"});
			continue;
		}

		const int mark = Mark(grid, cell);

		if (value < 0)
		{
			found.push_back({"stack",
				"cell " + key + " holds " + std::to_string(value) + ", which no body can have put there"});

			continue;
		}

		if (value != 0 && !expected.contains(key) && mark < Flags::CELL_COLLISION_MODE_HARD)
		{
			found.push_back({"stack",
				"cell " + key + " holds " + std::to_string(value) + " with nothing standing on it or booked over it"});

			continue;
		}

		if (mark < 0 || mark >= Flags::CELL_COLLISION_MODE_HARD)
		{
			continue;
		}

		int wanted = Flags::CELL_COLLISION_MODE_OFF;

		if (value >= Flags::CELL_COLLISION_MODE_MEDIUM)
		{
			wanted = Flags::CELL_COLLISION_MODE_MEDIUM;
		}
		else if (value > Flags::CELL_COLLISION_MODE_OFF)
		{
			wanted = Flags::CELL_COLLISION_MODE_SOFT;
		}

		if (mark != wanted)
		{
			found.push_back({"grid",
				"cell " + key + " holds " + std::to_string(value) + " but is marked " + std::to_string(mark) + " rather than " + std::to_string(wanted)});
		}
	}

	return found;
}

int Audit(std::int64_t tick)
{
	const Vector<Violation> found = Check();

	int reported = 0;

	for (const Violation &violation : found)
	{
		if (reported >= REPORTED)
		{
			Log::Error("Rules: " + std::to_string(static_cast<int>(found.size()) - reported) + " more at tick " + std::to_string(tick));
			break;
		}

		Log::Error("Rules: " + violation.rule + " at tick " + std::to_string(tick) + ": " + violation.detail);

		reported++;
	}

	return static_cast<int>(found.size());
}
} // namespace TGX::Rules
