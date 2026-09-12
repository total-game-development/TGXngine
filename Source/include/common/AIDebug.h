#pragma once

#include "Core.h"

namespace TGX
{
// What a commander is thinking about its purse, published every tick so the
// debug overlay can show whether the AI is really living within its means.
struct AIDebugSnapshot
{
	String team;
	String profile;

	int cash = 0;
	int cashSpent = 0;

	int structures = 0;
	int buildLimit = 0;

	int armySize = 0;
	int armyLimit = 0;

	int muster = 0;
	int waveSize = 0;
	int wavesSent = 0;

	// The order currently on the slab, and how far through its build time it is.
	String building;
	int buildProgress = 0;
	int buildTime = 0;

	// Why nothing is being built, when nothing is.
	String stalled;
};
} // namespace TGX
