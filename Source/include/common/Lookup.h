#pragma once

#include "WorldState.h"

namespace TGX
{
struct LookUp
{
	void static Add(int uid)
	{
		WorldState &world = WorldState::GetInstance();
		world.SetLookup(uid, static_cast<int>(world.items.size()));
	}

	int static Get(int uid)
	{
		WorldState &world = WorldState::GetInstance();

		auto &lookup = world.GetLookup();

		auto it = lookup.find(uid);
		if (it != lookup.end())
		{
			int index = it->second;
			if (index < static_cast<int>(world.items.size()) && uid == world.items[index]->GetUid())
			{
				return index;
			}
		}

		for (size_t i = 0; i < world.items.size(); i++)
		{
			if (world.items[i]->GetUid() == uid)
			{
				lookup[uid] = static_cast<int>(i);
				return static_cast<int>(i);
			}
		}

		return -1;
	}

	void static Set(int uid, int index)
	{
		WorldState &world = WorldState::GetInstance();
		world.SetLookup(uid, index);
	}

	// For a uid that was claimed but whose item never reached the world. Get()
	// survives a leftover entry -- it checks the uid at the index it finds and
	// falls back to a scan -- but it should not have to.
	void static Remove(int uid)
	{
		WorldState &world = WorldState::GetInstance();
		world.GetLookup().erase(uid);
	}
};
} // namespace TGX
