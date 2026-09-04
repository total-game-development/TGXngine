#pragma once

#include <nlohmann/json.hpp>
#include <mutex>
#include <thread>
#include "Trigger.h"

using namespace nlohmann;

namespace TGX
{
inline Unique<std::thread> trigger_thread;
inline bool trigger_running;

inline Vector<Unique<Trigger>> triggers;
inline std::mutex trigger_mutex;

inline int triggerIndex;
inline int triggerLimit = 10;

enum class Outcome : std::uint8_t
{
	Undecided,
	Won,
	Lost
};

void Run();
Outcome CurrentOutcome();
bool HasWon();
bool HasLost();
} // namespace TGX
