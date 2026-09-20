#pragma once

#include "Core.h"

namespace TGX
{
// Fills SkirmishSetup from maps.json so a match can be started from the command
// line instead of from the lobby. `map` is a map's name, or its place among the
// skirmish maps counted from one; empty takes the first. `team` is the side the
// player commands, and empty leaves the roles the map declares. Returns false,
// having listed what there is, when the map or the team is not one of them.
bool PrepareSkirmish(const String &map, const String &team);
} // namespace TGX
