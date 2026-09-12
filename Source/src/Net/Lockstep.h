#pragma once

#include <map>
#include <nlohmann/json.hpp>
#include "Core.h"

namespace TGX::Net
{
using Tick = std::int64_t;

struct Command
{
	Vector<int> uids;
	nlohmann::json orders;
};

// Holds this client's clock against the server's, and holds every command until
// the tick it was stamped for.
//
// The client never simulates past the last tick the server has told it about,
// which is what makes a stamped command impossible to receive late: the server
// only ever stamps for a tick at or beyond the one it last broadcast.
class Lockstep
{
private:
	// How far behind the last known server tick this client runs. The buffer
	// absorbs the round trip, so a command issued locally is already at every
	// peer by the time its tick comes up. Too small and the client stalls
	// between server updates; too large and orders feel sluggish.
	static constexpr Tick TARGET_BUFFER = 6;

	// A checksum goes up on this cadence. It must match what the server keeps,
	// and both sides key it by tick so lag cannot confuse the comparison.
	static constexpr Tick SANITY_INTERVAL = 60;

	std::map<Tick, Vector<Command>> pending;

	Tick localTick = 0;
	Tick serverTick = 0;

	bool running = false;

public:
	void Begin(Tick startTick);
	void Stop();

	void SetServerTick(Tick tick);
	void Accept(Tick stampedTick, const Vector<int> &uids, const nlohmann::json &orders);

	// True while the local clock may advance. The caller runs one simulation
	// step per call that returns true, draining Due() first.
	bool ShouldAdvance() const;

	// The commands stamped for the tick about to run. Taken, not copied: each
	// is applied exactly once.
	Vector<Command> Due();

	void Advance();

	bool IsSanityTick() const
	{
		return running && (localTick % SANITY_INTERVAL) == 0;
	}

	Tick LocalTick() const
	{
		return localTick;
	}

	Tick ServerTick() const
	{
		return serverTick;
	}

	// How far behind the server this client is sitting, for a readout.
	Tick Lag() const
	{
		return serverTick - localTick;
	}

	bool IsRunning() const
	{
		return running;
	}
};
} // namespace TGX::Net
