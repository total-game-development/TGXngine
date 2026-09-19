#pragma once

#include "Core.h"

namespace TGX
{
// Simulates one networked match with no window, beside the players in it. The
// server starts it with the room to host and a token that proves it was the
// one started; it joins as an observer does, runs every tick the players run,
// reports its checksums for the server to hold them against, and says which
// side is left standing. Returns when the match ends or the server goes away.
int RunHost(const String &url, int room, const String &token);
} // namespace TGX
