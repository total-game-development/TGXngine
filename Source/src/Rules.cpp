#include "Rules.h"

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

String Occupants(const GridTracker &tracker, const Map<int, const ItemInstance *> &live, const Cell &cell)
{
	String named;

	for (const auto &[uid, box] : tracker.uids_grid)
	{
		if (cell.x < std::get<0>(box) || cell.x > std::get<2>(box))
		{
			continue;
		}

		if (cell.y < std::get<1>(box) || cell.y > std::get<3>(box))
		{
			continue;
		}

		const auto found = live.find(uid);
		const String name = found == live.end() ? String{"dead"} : found->second->GetName();

		named += (named.empty() ? String{} : String{", "}) + name + "#" + std::to_string(uid);
	}

	return named.empty() ? String{"no body on the grid"} : named;
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

	for (const auto &[uid, box] : tracker.uids_grid)
	{
		if (!live.contains(uid))
		{
			found.push_back({"body",
				"uid " + std::to_string(uid) + " holds " + Key(std::get<0>(box), std::get<1>(box)) + " to " + Key(std::get<2>(box), std::get<3>(box)) + ", but nothing alive carries that uid"});
		}
	}

	for (const auto &[uid, reservation] : tracker.tactical_uids_grid)
	{
		if (!live.contains(uid))
		{
			found.push_back({"booking",
				"uid " + std::to_string(uid) + " has booked " + Key(reservation.x1, reservation.y1) + " to " + Key(reservation.x2, reservation.y2) + ", but nothing alive carries that uid"});
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

		if (value < 0)
		{
			found.push_back({"stack",
				"cell " + key + " holds " + std::to_string(value) + ", which no body can have put there: " + Occupants(tracker, live, cell)});
			continue;
		}

		const int hard = value / Flags::CELL_COLLISION_MODE_HARD;
		const int medium = (value % Flags::CELL_COLLISION_MODE_HARD) / Flags::CELL_COLLISION_MODE_MEDIUM;
		const int soft = value % Flags::CELL_COLLISION_MODE_MEDIUM;

		if (medium > 1)
		{
			found.push_back({"overlap",
				"cell " + key + " has " + std::to_string(medium) + " vehicles or ships stopped on it: " + Occupants(tracker, live, cell)});
		}

		if (medium > 0 && soft > 0)
		{
			found.push_back({"overlap",
				"cell " + key + " has infantry stopped where a vehicle or ship stands: " + Occupants(tracker, live, cell)});
		}

		if (hard > 0 && (medium > 0 || soft > 0))
		{
			found.push_back({"overlap",
				"cell " + key + " has a unit stopped inside a building or turret: " + Occupants(tracker, live, cell)});
		}

		if (hard > 1)
		{
			found.push_back({"overlap",
				"cell " + key + " has " + std::to_string(hard) + " buildings or turrets on it: " + Occupants(tracker, live, cell)});
		}
	}

	Map<String, int> expected;

	for (const auto &[uid, box] : tracker.uids_grid)
	{
		const auto carried = live.find(uid);

		if (carried == live.end())
		{
			continue;
		}

		const int mode = carried->second->GetCellCollisionMode();

		for (int x = std::get<0>(box); x <= std::get<2>(box); ++x)
		{
			for (int y = std::get<1>(box); y <= std::get<3>(box); ++y)
			{
				const Cell cell{x, y};
				const int mark = Mark(grid, cell);

				// A cell already hard when the body was placed took none of
				// it, so it is not the grid's to account for here.
				if (mark < 0 || (mark >= Flags::CELL_COLLISION_MODE_HARD && mode < Flags::CELL_COLLISION_MODE_HARD))
				{
					continue;
				}

				expected[Key(x, y)] += mode;
			}
		}
	}

	for (const auto &[key, value] : expected)
	{
		const auto held = tracker.cells_grid.find(key);
		const int actual = held == tracker.cells_grid.end() ? 0 : held->second;

		if (actual != value)
		{
			Cell cell;
			Parse(key, cell);

			found.push_back({"stack",
				"cell " + key + " holds " + std::to_string(actual) + " but carries bodies worth " + std::to_string(value) + ": " + Occupants(tracker, live, cell)});
		}
	}

	for (const auto &[key, value] : tracker.cells_grid)
	{
		if (value == 0 || expected.contains(key))
		{
			continue;
		}

		Cell cell;

		if (!Parse(key, cell))
		{
			continue;
		}

		if (Mark(grid, cell) >= Flags::CELL_COLLISION_MODE_HARD)
		{
			continue;
		}

		found.push_back({"stack", "cell " + key + " holds " + std::to_string(value) + " with no body on it"});
	}

	for (const auto &[key, value] : tracker.cells_grid)
	{
		Cell cell;

		if (!Parse(key, cell) || value < 0)
		{
			continue;
		}

		const int mark = Mark(grid, cell);

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
