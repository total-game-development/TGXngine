#include "Triggers.h"
#include <vector>
#include "Enums.h"
#include "module_interface.h"

namespace TGX
{
extern "C"
{
	MODULE_API void Init()
	{
		Log::Success("Triggers initialized");
	}

	MODULE_API void Awake(const String &name)
	{
		Function<bool()> wonTest = [] { return HasWon(); };
		Function<bool()> lostTest = [] { return HasLost(); };

		triggers.push_back(std::make_unique<ConditionalTrigger>(UIAction::GameOver, "won", wonTest));
		triggers.push_back(std::make_unique<ConditionalTrigger>(UIAction::GameOver, "lost", lostTest));

		Log::Success("Trigger created: " + name + ", number of triggers: " + std::to_string(triggers.size()));
	}

	MODULE_API void Start()
	{
		triggerCountdown = TRIGGER_INTERVAL;
	}

	// Driven by the scene, once for every tick it steps. A trigger that fires is
	// dropped, as it was before: the conditions here are the end of the match and
	// there is nothing to ask after the answer.
	MODULE_API void Update()
	{
		if (triggers.empty())
		{
			return;
		}

		if (triggerCountdown > 0)
		{
			triggerCountdown--;
			return;
		}

		triggerCountdown = TRIGGER_INTERVAL;

		std::erase_if(triggers, [](const Unique<Trigger> &trigger) {
			return trigger->Elapse();
		});
	}

	MODULE_API void Clear()
	{
		triggers.clear();

		triggerCountdown = TRIGGER_INTERVAL;
	}

	MODULE_API void Delete()
	{
		Log::Clean("Deleting triggers...");
		Clear();
		Log::Success("All triggers deleted");
	}
}

Outcome CurrentOutcome()
{
	WorldState &world = WorldState::GetInstance();

	bool mine = false;
	bool theirs = false;

	for (const auto &item : world.items)
	{
		if (!item)
		{
			continue;
		}

		if (item->GetTeam() == world.GetTeam())
		{
			mine = true;
		}
		else
		{
			theirs = true;
		}

		if (mine && theirs)
		{
			return Outcome::Undecided;
		}
	}

	if (!mine && !theirs)
	{
		return Outcome::Undecided;
	}

	return theirs ? Outcome::Lost : Outcome::Won;
}

bool HasWon()
{
	if (CurrentOutcome() != Outcome::Won)
	{
		return false;
	}

	Log::Success("Match over: " + WorldState::GetInstance().GetTeam() + " holds the map");

	return true;
}

bool HasLost()
{
	if (CurrentOutcome() != Outcome::Lost)
	{
		return false;
	}

	Log::Warning("Match over: " + WorldState::GetInstance().GetTeam() + " has nothing left");

	return true;
}
} // namespace TGX
