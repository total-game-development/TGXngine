#include "Triggers.h"
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

		std::lock_guard<std::mutex> lock(trigger_mutex);
		triggers.push_back(std::make_unique<ConditionalTrigger>(UIAction::GameOver, "won", wonTest));
		triggers.push_back(std::make_unique<ConditionalTrigger>(UIAction::GameOver, "lost", lostTest));

		Log::Success("Trigger created: " + name + ", number of triggers: " + std::to_string(triggers.size()));
	}

	MODULE_API void Start()
	{
		triggerIndex = 0;
		trigger_running = true;

		trigger_thread = std::make_unique<std::thread>([] {
			Run();
		});
	}

	MODULE_API void Clear()
	{
		trigger_running = false;

		if (trigger_thread && trigger_thread->joinable())
		{
			trigger_thread->join();
		}

		trigger_thread.reset();

		std::lock_guard<std::mutex> lock(trigger_mutex);
		triggers.clear();

		triggerIndex = 0;
	}

	MODULE_API void Delete()
	{
		Log::Clean("Deleting triggers...");
		Clear();
		Log::Success("All triggers deleted");
	}
}

void Run()
{
	while (trigger_running)
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));

		std::lock_guard<std::mutex> lock(trigger_mutex);

		if (triggers.empty())
		{
			continue;
		}

		if (triggerIndex >= static_cast<int>(triggers.size()))
		{
			triggerIndex = 0;
		}

		if (triggers[triggerIndex]->Elapse())
		{
			triggers.erase(triggers.begin() + triggerIndex);
		}
		else
		{
			triggerIndex++;
		}
	}

	Log::Clean("Trigger thread stopped");
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
