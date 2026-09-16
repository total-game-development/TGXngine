#pragma once

#include <algorithm>
#include "Core.h"

namespace TGX
{
class EconomyInstance
{
private:
	String team = "none";
	int cash = 0;
	Map<String, float> resources;

public:
	EconomyInstance() = default;

	EconomyInstance(const EconomyInstance &) = delete;
	EconomyInstance &operator=(const EconomyInstance &) = delete;

	virtual ~EconomyInstance() = default;

	String GetTeam() const { return team; }
	void SetTeam(const String &inTeam) { team = inTeam; }

	int GetCash() const { return cash; }
	void SetCash(int inCash) { cash = inCash; }
	void AddCash(int amount) { cash += amount; }

	void AddResourceProgress(const String &resName, float amount)
	{
		resources[resName] += amount;
	}

	float GetResourceProgress(const String &resName)
	{
		return resources[resName];
	}

	void ConsumeResourceProgress(const String &resName, float threshold)
	{
		resources[resName] -= threshold;
	}

	// Sorted by name, because the container behind it is hashed and hands its
	// entries back in whatever order it likes. A digest folded in that order
	// would differ between two clients holding identical progress.
	Vector<Pair<String, float>> OrderedProgress() const
	{
		Vector<Pair<String, float>> ordered(resources.begin(), resources.end());

		std::sort(ordered.begin(), ordered.end(), [](const auto &a, const auto &b) {
			return a.first < b.first;
		});

		return ordered;
	}
};
} // namespace TGX
