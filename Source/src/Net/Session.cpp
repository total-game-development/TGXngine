#include "Session.h"
#include "Logs.h"
#include "MultiplayerSetup.h"
#include "Renderer.h"

namespace TGX::Net
{
Session &Session::GetInstance()
{
	static Session instance;

	return instance;
}

void Session::Connect(const String &inUrl)
{
	url = inUrl;

	rooms.clear();
	levels.clear();
	state = RoomState{};

	notice.clear();
	team.clear();
	token.clear();

	room = -1;
	slot = -1;

	attempts = 0;
	cooldown = 0;
	observer = false;
	held = false;

	replay.clear();

	MultiplayerSetup::Clear();

	phase = Phase::Connecting;
	interrupted = Phase::Offline;

	client.Connect(url);
}

void Session::Disconnect()
{
	lockstep.Stop();
	client.Disconnect();

	phase = Phase::Offline;
	interrupted = Phase::Offline;

	rooms.clear();
	state = RoomState{};

	team.clear();
	token.clear();

	room = -1;
	slot = -1;

	held = false;
	replay.clear();
	shell.clear();
}

void Session::RequestRooms()
{
	client.Send({{"type", "init_lobby"}});
}

void Session::Join(int roomId, bool asObserver)
{
	room = roomId;
	observer = asObserver;

	MultiplayerSetup::observer = asObserver;

	client.Send({{"type", "join_game"},
				 {"id", roomId},
				 {"slot", -1},
				 {"platform", asObserver ? "observer" : "desktop_player"}});

	phase = Phase::Waiting;
	notice = "Joining room " + std::to_string(roomId + 1) + "...";
}

void Session::JoinAsHost(int roomId, const String &hostToken)
{
	room = roomId;
	observer = true;

	MultiplayerSetup::observer = true;

	client.Send({{"type", "join_game"},
				 {"id", roomId},
				 {"slot", -1},
				 {"platform", "host"},
				 {"token", hostToken}});

	phase = Phase::Waiting;
	notice = "Hosting room " + std::to_string(roomId + 1) + "...";
}

void Session::JoinArena(int roomId)
{
	room = roomId;
	observer = true;

	MultiplayerSetup::observer = true;

	client.Send({{"type", "join_arena"},
				 {"id", roomId},
				 {"slot", -1},
				 {"platform", "observer"}});

	phase = Phase::Waiting;
	notice = "Opening an arena in room " + std::to_string(roomId + 1) + "...";
}

void Session::ReportOutcome(const String &outcome)
{
	if (!IsPlaying())
	{
		return;
	}

	client.Send({{"type", "outcome"}, {"outcome", outcome}, {"tick", lockstep.LocalTick()}});
}

// The same join, with the token that names the seat already held. The server
// gives the seat back rather than handing out a free one, and answers with the
// match as it stands instead of refusing a room that is playing.
void Session::Rejoin()
{
	client.Send({{"type", "join_game"},
				 {"id", room},
				 {"slot", slot},
				 {"resume", token},
				 {"platform", observer ? "observer" : "desktop_player"}});
}

void Session::Leave()
{
	if (room < 0)
	{
		return;
	}

	client.Send({{"type", "leave"}});

	lockstep.Stop();

	room = -1;
	slot = -1;

	token.clear();
	team.clear();

	state = RoomState{};
	held = false;

	phase = Phase::Lobby;

	RequestRooms();
}

void Session::ChooseSlot(int index)
{
	client.Send({{"type", "set_slot"}, {"slot", index}});
}

void Session::ChooseTeam(const String &name)
{
	client.Send({{"type", "set_team"}, {"team", name}});
}

void Session::ChooseLevel(std::size_t index)
{
	client.Send({{"type", "set_level"}, {"level", index}});
}

void Session::SetReady(bool ready)
{
	client.Send({{"type", "set_ready"}, {"ready", ready}});
}

void Session::SendCommand(const Vector<int> &uids, const nlohmann::json &orders)
{
	if (!IsPlaying())
	{
		return;
	}

	// Asked for far enough ahead that it reaches every peer before its tick
	// comes up. The server re-stamps anything it cannot honour.
	client.Send({{"type", "command"},
				 {"uids", uids},
				 {"tick", lockstep.ServerTick() + 2},
				 {"orders", orders}});
}

void Session::ReportDigest(std::uint64_t world, std::uint64_t commands)
{
	if (!IsPlaying())
	{
		return;
	}

	client.Send({{"type", "sanity_check"},
				 {"tick", lockstep.LocalTick()},
				 {"value", world},
				 {"commands", commands}});
}

void Session::SendShell(const String &to, const nlohmann::json &body)
{
	if (!IsPlaying())
	{
		return;
	}

	client.Send({{"type", "shell"}, {"to", to}, {"body", body}});
}

Vector<nlohmann::json> Session::TakeShell()
{
	Vector<nlohmann::json> taken;
	taken.swap(shell);

	return taken;
}

Vector<nlohmann::json> Session::TakeReplay()
{
	Vector<nlohmann::json> taken;
	taken.swap(replay);

	return taken;
}

// A socket lost while a seat is held is worth another try: the server holds the
// match for its grace period, so a blip costs a pause rather than the match.
bool Session::Retry()
{
	if (token.empty() || attempts >= RECONNECT_ATTEMPTS)
	{
		return false;
	}

	interrupted = phase == Phase::Resuming ? interrupted : phase;
	phase = Phase::Resuming;

	if (cooldown > 0)
	{
		cooldown--;
		return true;
	}

	attempts++;
	cooldown = RECONNECT_COOLDOWN;

	notice = "Connection lost. Reconnecting (" + std::to_string(attempts) + "/" + std::to_string(RECONNECT_ATTEMPTS) + ")...";

	Log::Warning(notice);

	client.Connect(url);

	return true;
}

void Session::Poll()
{
	const Status status = client.GetStatus();

	if (status == Status::Failed)
	{
		if (Retry())
		{
			return;
		}

		if (phase != Phase::Refused)
		{
			notice = "Connection failed: " + client.Failure();
			Log::Error(notice);
		}

		phase = Phase::Refused;
		return;
	}

	if (status == Status::Closed && phase != Phase::Offline && phase != Phase::Ended)
	{
		if (Retry())
		{
			return;
		}

		notice = "Connection closed by the server.";
		phase = Phase::Ended;
		lockstep.Stop();
		return;
	}

	if (phase == Phase::Resuming && status == Status::Connected)
	{
		Log::Info("Reconnected; asking for the seat back");

		Rejoin();

		// Not Playing again until the server has said so. Until then the clock
		// stays where it stopped and nothing is simulated.
		notice = "Reconnected. Rejoining the match...";
		return;
	}

	if (phase == Phase::Connecting && status == Status::Connected)
	{
		phase = Phase::Lobby;
		notice = "Connected.";
		attempts = 0;

		RequestRooms();
	}

	for (const nlohmann::json &message : client.Drain())
	{
		Handle(message);
	}
}

void Session::EnterMatch(const nlohmann::json &message, bool resuming)
{
	team = message.value("team", String{});
	slot = message.value("slot", -1);

	if (message.contains("token") && message["token"].is_string())
	{
		token = message["token"].get<String>();
	}

	MultiplayerSetup::active = true;
	MultiplayerSetup::team = team;
	MultiplayerSetup::seed = message.value("seed", std::uint32_t{0});
	MultiplayerSetup::startTick = message.value("tick", std::int64_t{0});
	MultiplayerSetup::level = message.value("currentLevel", nlohmann::json::object());
	MultiplayerSetup::observer = observer;
	MultiplayerSetup::aiSides = message.value("ai", Vector<String>{});

	replay.clear();

	for (const auto &entry : message.value("commands", nlohmann::json::array()))
	{
		replay.push_back(entry);
	}

	lockstep.Begin(MultiplayerSetup::startTick);
	lockstep.SetServerTick(message.value("serverTick", MultiplayerSetup::startTick));

	phase = Phase::Playing;
	attempts = 0;
	cooldown = 0;
	held = false;

	notice = resuming
				 ? "Rejoined as " + team + "; replaying " + std::to_string(replay.size()) + " command(s)"
				 : "Match started as " + team;

	Log::Success(notice + " on " + MultiplayerSetup::level.value("name", String{"?"}));

	// The scene is loaded fresh either way. A returning client has no world left
	// to patch up, so it builds the match from the seed and replays its way back
	// to the tick the rest of the room is on.
	Renderer::GetInstance().LoadScene(SceneType::Game);
}

void Session::Handle(const nlohmann::json &message)
{
	const String type = message.value("type", String{});

	if (type == "room_list")
	{
		rooms.clear();

		for (const auto &entry : message.value("rooms", nlohmann::json::array()))
		{
			RoomSummary summary;

			summary.number = entry.value("number", 0);
			summary.status = entry.value("status", String{"?"});
			summary.levelName = entry.value("levelName", String{});
			summary.occupied = entry.value("occupied", 0);
			summary.capacity = entry.value("capacity", 0);
			summary.level = entry.value("level", std::size_t{0});
			summary.running = entry.value("running", false);

			rooms.push_back(summary);
		}

		levels.clear();

		for (const auto &entry : message.value("levels", nlohmann::json::array()))
		{
			levels.push_back(entry.is_string() ? entry.get<String>() : String{"?"});
		}

		return;
	}

	if (type == "room_state")
	{
		state = RoomState{};

		state.number = message.value("room", 0);
		state.level = message.value("level", std::size_t{0});
		state.levelName = message.value("levelName", String{});
		state.status = message.value("status", String{});
		state.observers = message.value("observers", 0);
		state.running = message.value("running", false);
		state.canStart = message.value("canStart", false);

		for (const auto &entry : message.value("levels", nlohmann::json::array()))
		{
			state.levels.push_back(entry.is_string() ? entry.get<String>() : String{"?"});
		}

		for (const auto &entry : message.value("teams", nlohmann::json::array()))
		{
			state.teams.push_back(entry.is_string() ? entry.get<String>() : String{"?"});
		}

		for (const auto &entry : message.value("slots", nlohmann::json::array()))
		{
			SlotState seat;

			seat.index = entry.value("index", 0);
			seat.team = entry.value("team", String{});
			seat.occupied = entry.value("occupied", false);
			seat.connected = entry.value("connected", false);
			seat.ready = entry.value("ready", false);
			seat.mine = entry.value("mine", false);

			state.slots.push_back(seat);
		}

		if (state.Mine() >= 0)
		{
			slot = state.Mine();
			team = state.slots[static_cast<std::size_t>(slot)].team;
		}

		if (phase == Phase::Waiting || phase == Phase::Lobby)
		{
			phase = Phase::Room;
		}

		return;
	}

	if (type == "joined_game")
	{
		notice = message.value("status", String{"Waiting for another player..."});

		slot = message.value("slot", -1);

		if (message.contains("token") && message["token"].is_string())
		{
			token = message["token"].get<String>();
		}

		if (message.contains("team") && message["team"].is_string())
		{
			team = message["team"].get<String>();
		}

		phase = Phase::Room;
		return;
	}

	if (type == "join_refused")
	{
		notice = "Refused: " + message.value("reason", String{"unknown"});

		// A refusal during a resume means the seat is gone, so there is nothing
		// to go back to. Everything else leaves the lobby as it was.
		token.clear();
		phase = interrupted == Phase::Playing ? Phase::Ended : Phase::Refused;

		lockstep.Stop();

		Log::Warning(notice);
		return;
	}

	if (type == "start_game")
	{
		EnterMatch(message, false);
		return;
	}

	if (type == "resume_game")
	{
		EnterMatch(message, true);
		return;
	}

	if (type == "server_tick")
	{
		lockstep.SetServerTick(message.value("tick", std::int64_t{0}));
		return;
	}

	if (type == "commands")
	{
		lockstep.SetServerTick(message.value("serverTick", std::int64_t{0}));

		for (const auto &entry : message.value("commands", nlohmann::json::array()))
		{
			lockstep.Accept(
				entry.value("tick", std::int64_t{0}),
				entry.value("uids", Vector<int>{}),
				entry.value("orders", nlohmann::json::object()));
		}

		return;
	}

	if (type == "match_held")
	{
		held = true;
		notice = "Match held: waiting for a player to return.";

		Log::Warning(notice);
		return;
	}

	if (type == "match_resumed")
	{
		held = false;
		notice = "Match resumed.";

		Log::Success(notice);
		return;
	}

	if (type == "player_dropped")
	{
		notice = message.value("team", String{"A player"}) + " dropped; holding for " + std::to_string(message.value("grace", 0)) + "s.";

		Log::Warning(notice);
		return;
	}

	if (type == "player_resumed")
	{
		notice = message.value("team", String{"A player"}) + " is back.";
		return;
	}

	if (type == "desync")
	{
		Log::Error("Server reports a desync at tick " + std::to_string(message.value("tick", std::int64_t{0})));
		notice = "Desynchronised from the server.";
		return;
	}

	if (type == "end_game")
	{
		notice = "Match ended: " + message.value("outcome", String{"over"});

		phase = Phase::Ended;
		held = false;
		token.clear();

		lockstep.Stop();

		Log::Success(notice);
		return;
	}

	if (type == "player_left")
	{
		notice = "A player left the match.";
		return;
	}

	if (type == "shell" || type == "shell_refused")
	{
		shell.push_back(message);
		return;
	}

	if (type == "command_stamped" || type == "pong" || type == "chat")
	{
		return;
	}

	Log::Warning("Unhandled server message: " + type);
}
} // namespace TGX::Net
