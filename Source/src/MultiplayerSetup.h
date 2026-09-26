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

	// Joined to break in rather than to play. A hacker holds no side and owns
	// nothing on the board, but it has a console the room gave a name to, and
	// the other players' computers are reachable from it.
	static inline bool hacker = false;

	// What this client answers to on the shell network: the side it holds, or
	// the name the room handed a hacker.
	static inline String console;

	// The consoles of everybody watching an arena on a cyber map, this one's
	// among them. Viewers come and go, so the server says again as they do.
	static inline Vector<String> consoles;

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
		hacker = false;
		console.clear();
		aiSides.clear();
		consoles.clear();
	}
};
} // namespace TGX
