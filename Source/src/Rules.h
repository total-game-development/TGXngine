#pragma once

#include "Core.h"

namespace TGX::Rules
{
struct Violation
{
	String rule;
	String detail;
};

// What the stopped world may never look like, as opposed to what merely
// differs from another client. The occupancy grid carries a body only while
// its unit stands still, so anything overlapping there has come to rest on
// ground another unit already holds.
Vector<Violation> Check();

// Checks, reports what it found against the tick, and returns how many.
int Audit(std::int64_t tick);
} // namespace TGX::Rules
