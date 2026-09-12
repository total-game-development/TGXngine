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

void Session::Connect(const String &url)
{
	rooms.clear();
	notice.clear();
	team.clear();
	room = -1;

	MultiplayerSetup::Clear();

	phase = Phase::Connecting;

	client.Connect(url);
}

void Session::Disconnect()
{
	lockstep.Stop();
	client.Disconnect();

	phase = Phase::Offline;

	rooms.clear();
	team.clear();
	room = -1;
}

void Session::RequestRooms()
{
	client.Send({{"type", "init_lobby"}});
}

void Session::Join(int roomId, bool asObserver)
{
	room = roomId;

	MultiplayerSetup::observer = asObserver;

	client.Send({
		{"type", "join_game"},
		{"id", roomId},
		{"platform", asObserver ? "observer" : "desktop_player"}});

	phase = Phase::Waiting;
	notice = "Joining room " + std::to_string(roomId + 1) + "...";
}

void Session::SendCommand(const Vector<int> &uids, const nlohmann::json &orders)
{
	if (!IsPlaying())
	{
		return;
	}

	// Asked for far enough ahead that it reaches every peer before its tick
	// comes up. The server re-stamps anything it cannot honour.
	client.Send({
		{"type", "command"},
		{"uids", uids},
		{"tick", lockstep.ServerTick() + 2},
		{"orders", orders}});
}

void Session::ReportDigest(std::uint64_t value)
{
	digest = value;

	if (!IsPlaying())
	{
		return;
	}

	client.Send({
		{"type", "sanity_check"},
		{"tick", lockstep.LocalTick()},
		{"value", value}});
}

void Session::Poll()
{
	const Status status = client.GetStatus();

	if (status == Status::Failed)
	{
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
		notice = "Connection closed by the server.";
		phase = Phase::Ended;
		lockstep.Stop();
		return;
	}

	if (phase == Phase::Connecting && status == Status::Connected)
	{
		phase = Phase::Lobby;
		notice = "Connected.";

		RequestRooms();
	}

	for (const nlohmann::json &message : client.Drain())
	{
		Handle(message);
	}
}

void Session::Handle(const nlohmann::json &message)
{
	const String type = message.value("type", String{});

	if (type == "room_list")
	{
		rooms.clear();

		for (const auto &entry : message.value("rooms", nlohmann::json::array()))
		{
			rooms.push_back(entry.is_string() ? entry.get<String>() : String{"?"});
		}

		return;
	}

	if (type == "joined_game")
	{
		notice = message.value("status", String{"Waiting for another player..."});
		phase = Phase::Waiting;
		return;
	}

	if (type == "join_refused")
	{
		notice = "Refused: " + message.value("reason", String{"unknown"});
		phase = Phase::Refused;

		Log::Warning(notice);
		return;
	}

	if (type == "start_game")
	{
		team = message.value("team", String{});

		MultiplayerSetup::active = true;
		MultiplayerSetup::team = team;
		MultiplayerSetup::seed = message.value("seed", std::uint32_t{0});
		MultiplayerSetup::startTick = message.value("tick", std::int64_t{0});
		MultiplayerSetup::level = message.value("currentLevel", nlohmann::json::object());

		lockstep.Begin(MultiplayerSetup::startTick);

		phase = Phase::Playing;
		notice = "Match started as " + team;

		Log::Success(notice + " on " + MultiplayerSetup::level.value("name", String{"?"}));

		Renderer::GetInstance().LoadScene(SceneType::Game);
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
		lockstep.Stop();

		Log::Success(notice);
		return;
	}

	if (type == "player_left")
	{
		notice = "A player left the match.";
		return;
	}

	if (type == "command_stamped" || type == "pong" || type == "chat")
	{
		return;
	}

	Log::Warning("Unhandled server message: " + type);
}
} // namespace TGX::Net
