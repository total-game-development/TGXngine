#pragma once

#include "Core.h"
#include "Flags.h"
#include "ItemInstance.h"
#include "WorldState.h"

namespace TGX
{
struct BuildSpace
{
	static BuildSpan Of(const String &kind, int fallbackWidth, int fallbackHeight)
	{
		WorldState &world = WorldState::GetInstance();

		const auto span = world.buildSpans.find(kind);

		if (span == world.buildSpans.end())
		{
			return {fallbackWidth, fallbackHeight};
		}

		return {std::max(span->second.width, fallbackWidth), std::max(span->second.height, fallbackHeight)};
	}

	// The footprint is held to `blocking`, since a player puts a building on
	// clear ground; the deploy band below it only has to be free of anything
	// solid, or a unit standing in the way would refuse every plot on the map.
	static bool Clear(const String &kind, int x, int y, int footprintWidth, int footprintHeight, int blocking)
	{
		WorldState &world = WorldState::GetInstance();

		const BuildSpan span = Of(kind, footprintWidth, footprintHeight);

		if (span.width <= 0 || span.height <= 0)
		{
			return false;
		}

		if (x < 0 || y < 0 ||
			x + span.width > world.GetMapGridWidth() ||
			y + span.height > world.GetMapGridHeight())
		{
			return false;
		}

		for (int cellY = y; cellY < y + span.height; cellY++)
		{
			for (int cellX = x; cellX < x + span.width; cellX++)
			{
				const bool footprint = (cellX < x + footprintWidth) && (cellY < y + footprintHeight);
				const int refuse = footprint ? blocking : Flags::CELL_COLLISION_MODE_HARD;

				if (world.currentTerrainMapPassableGrid[cellY][cellX] >= refuse)
				{
					return false;
				}
			}
		}

		return !Claimed(x, y, span);
	}

	// Ground another building already needs. Its footprint is hard and caught
	// above; this is the band it deploys into, which nothing marks.
	static bool Claimed(int x, int y, const BuildSpan &span)
	{
		WorldState &world = WorldState::GetInstance();

		for (const auto &item : world.items)
		{
			if (!item || item->GetLife() <= 0.0f || item->GetType() != "buildings")
			{
				continue;
			}

			const auto held = world.buildSpans.find(item->GetName());

			if (held == world.buildSpans.end())
			{
				continue;
			}

			const int otherX = static_cast<int>(item->GetX());
			const int otherY = static_cast<int>(item->GetY());

			if (x + span.width <= otherX || otherX + held->second.width <= x)
			{
				continue;
			}

			if (y + span.height <= otherY || otherY + held->second.height <= y)
			{
				continue;
			}

			return true;
		}

		return false;
	}
};
} // namespace TGX
