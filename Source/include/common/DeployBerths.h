#pragma once

#include <climits>
#include "Core.h"
#include "ItemInstance.h"
#include "Lookup.h"
#include "WorldState.h"

namespace TGX
{
struct DeployBerths
{
	void static ReleaseVacated(const ItemInstance *producer, Vector<Tuple<float, float, int>> &berths)
	{
		if (!producer)
		{
			return;
		}

		WorldState &world = WorldState::GetInstance();

		for (auto &berth : berths)
		{
			const int occupant = std::get<2>(berth);

			if (occupant == INT_MIN)
			{
				continue;
			}

			const int occupantIndex = LookUp::Get(occupant);

			if (occupantIndex == -1)
			{
				std::get<2>(berth) = INT_MIN;
				continue;
			}

			const ItemInstance *unit = world.items[occupantIndex].get();

			const float offsetX = unit->GetX() - (producer->GetX() + std::get<0>(berth));
			const float offsetY = unit->GetY() - (producer->GetY() + std::get<1>(berth));

			if (((offsetX * offsetX) + (offsetY * offsetY)) > 1.0f)
			{
				std::get<2>(berth) = INT_MIN;
			}
		}
	}
};
} // namespace TGX
