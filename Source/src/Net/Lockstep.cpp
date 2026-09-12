#include "Lockstep.h"
#include "Logs.h"

namespace TGX::Net
{
void Lockstep::Begin(Tick startTick)
{
	pending.clear();

	localTick = startTick;
	serverTick = startTick;
	running = true;

	Log::Success("Lockstep started at tick " + std::to_string(startTick));
}

void Lockstep::Stop()
{
	pending.clear();

	running = false;
}

void Lockstep::SetServerTick(Tick tick)
{
	if (tick > serverTick)
	{
		serverTick = tick;
	}
}

void Lockstep::Accept(Tick stampedTick, const Vector<int> &uids, const nlohmann::json &orders)
{
	if (stampedTick < localTick)
	{
		// The client is not allowed to have run past a stamp. If this fires the
		// pacing is wrong, and applying it now would desync rather than repair.
		Log::Error("Command stamped for tick " + std::to_string(stampedTick) + " but the local clock is already at " + std::to_string(localTick));
		return;
	}

	pending[stampedTick].push_back({uids, orders});
}

bool Lockstep::ShouldAdvance() const
{
	return running && localTick < (serverTick - TARGET_BUFFER);
}

Vector<Command> Lockstep::Due()
{
	const auto found = pending.find(localTick);

	if (found == pending.end())
	{
		return {};
	}

	Vector<Command> commands = std::move(found->second);

	pending.erase(found);

	return commands;
}

void Lockstep::Advance()
{
	localTick++;
}
} // namespace TGX::Net
