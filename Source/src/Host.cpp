#include "Host.h"
#include <chrono>
#include <thread>
#include "MultiplayerSetup.h"
#include "Net/Session.h"
#include "Renderer.h"
#include "Rules.h"
#include "Scene/Game.h"
#include "WorldState.h"

namespace TGX
{
namespace
{
// Long enough not to spin a core, short enough that the host never falls a
// tick behind the clock it is keeping pace with.
constexpr std::chrono::milliseconds IDLE{2};

// Every side still holding anything, in the order the level lists them.
Vector<String> Standing()
{
	WorldState &world = WorldState::GetInstance();

	Vector<String> standing;

	for (const auto &side : MultiplayerSetup::level.value("teams", nlohmann::json::array()))
	{
		const String team = side.value("name", String{});

		for (const auto &item : world.items)
		{
			if (item && item->GetTeam() == team && item->GetLife() > 0.0f)
			{
				standing.push_back(team);
				break;
			}
		}
	}

	return standing;
}
} // namespace

int RunHost(const String &url, int room, const String &token, int audit)
{
	// Set before the renderer exists, since building it builds the window.
	// Nothing moves a pointer here, so park it off the map where no unit can
	// be under it.
	WorldState &world = WorldState::GetInstance();
	world.SetHeadless(true);

	MultiplayerSetup::host = true;
	world.SetGameX(-1.0e6f);
	world.SetGameY(-1.0e6f);

	Renderer &renderer = Renderer::GetInstance();
	Net::Session &session = Net::Session::GetInstance();

	Log::Info("Hosting room " + std::to_string(room + 1) + " on " + url);

	session.Connect(url);

	bool joined = false;
	bool played = false;
	bool decided = false;

	int violations = 0;
	int checks = 0;
	std::int64_t audited = -1;

	while (true)
	{
		const Net::Session::Phase phase = session.GetPhase();

		if (phase == Net::Session::Phase::Refused || phase == Net::Session::Phase::Ended)
		{
			break;
		}

		if (!session.IsPlaying())
		{
			session.Poll();

			if (!joined && session.GetPhase() == Net::Session::Phase::Lobby)
			{
				session.JoinAsHost(room, token);
				joined = true;
			}

			std::this_thread::sleep_for(IDLE);
			continue;
		}

		played = true;

		renderer.GetGame()->AdvanceNetworked();

		if (audit > 0 && session.IsPlaying() && session.Clock().IsCaughtUp())
		{
			const std::int64_t tick = session.Clock().LocalTick();

			if (tick != audited && (tick % audit) == 0)
			{
				audited = tick;
				checks++;
				violations += Rules::Audit(tick);
			}
		}

		// Once a side has nothing left, the match is over for it. With one
		// standing that side has won; with none, nobody has.
		if (!decided && session.IsPlaying() && session.Clock().IsCaughtUp())
		{
			const Vector<String> standing = Standing();

			if (standing.size() <= 1)
			{
				decided = true;

				const String outcome = standing.empty() ? String{"draw"} : standing.front() + " wins";

				Log::Success("Room " + std::to_string(room + 1) + ": " + outcome);

				session.ReportOutcome(outcome);
			}
		}

		std::this_thread::sleep_for(IDLE);
	}

	if (played && audit > 0)
	{
		const String tally = "Rules: " + std::to_string(violations) + " violations over " + std::to_string(checks) + " checks";

		if (violations > 0)
		{
			Log::Error(tally);
		}
		else
		{
			Log::Success(tally);
		}
	}

	Log::Info("Host for room " + std::to_string(room + 1) + " stopping: " + session.Notice());

	return played ? 0 : 1;
}
} // namespace TGX
