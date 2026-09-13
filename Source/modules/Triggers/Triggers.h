#pragma once

#include <nlohmann/json.hpp>
#include "Trigger.h"

using namespace nlohmann;

namespace TGX
{
inline Vector<Unique<Trigger>> triggers;

// Ticks between one sweep of the conditions and the next. Counted in ticks
// rather than slept on a wall clock: the simulation advances in ticks, so a
// check paced by the machine's clock lands on a different tick on every client,
// and the match is declared over at two different moments. Every other read of
// the world happens on the tick that owns it, and this is no different.
inline constexpr int TRIGGER_INTERVAL = 60;

inline int triggerCountdown = TRIGGER_INTERVAL;

enum class Outcome : std::uint8_t
{
	Undecided,
	Won,
	Lost
};

Outcome CurrentOutcome();
bool HasWon();
bool HasLost();
} // namespace TGX
