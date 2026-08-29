#pragma once

#include "Core.h"

namespace TGX
{
// Exe-only state: the lobby writes it, Game::Init reads it. Static members in a
// header are one copy per binary, so a module DLL would only ever see an empty one.
struct SkirmishSetup
{
	struct Slot
	{
		String team;
		String role;
	};

	static inline bool active = false;
	static inline int level = -1;
	static inline Vector<Slot> slots;
	static inline bool spectator = false;

	static void Clear()
	{
		active = false;
		level = -1;
		spectator = false;
		slots.clear();
	}

	static String PlayerTeam()
	{
		for (const Slot &slot : slots)
		{
			if (slot.role == "player")
			{
				return slot.team;
			}
		}

		return {};
	}
};
} // namespace TGX
