#pragma once

#include <nlohmann/json.hpp>
#include "Core.h"

namespace TGX
{
// Exe-only state, as SkirmishSetup is: the lobby writes it and Game::Init reads
// it. Static members in a header are one copy per binary, so a module DLL would
// only ever see an empty one.
struct MultiplayerSetup
{
	static inline bool active = false;

	static inline String team;
	static inline std::uint32_t seed = 0;
	static inline std::int64_t startTick = 0;

	// The level the server dealt, rather than one chosen locally: every client
	// must build the match from the same description.
	static inline nlohmann::json level;

	static inline bool observer = false;

	// The sides nobody sits on, which the match's host commands. Every other
	// client treats them as it treats any other player it cannot see.
	static inline Vector<String> aiSides;

	// True only in the process hosting the match: the one place its AI runs.
	static inline bool host = false;

	static void Clear()
	{
		active = false;
		team.clear();
		seed = 0;
		startTick = 0;
		level = nlohmann::json();
		observer = false;
		aiSides.clear();
	}
};
} // namespace TGX
