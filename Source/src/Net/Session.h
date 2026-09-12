#pragma once

#include "Client.h"
#include "Lockstep.h"

namespace TGX::Net
{
// The one place that speaks the server's protocol. The lobby drives it before a
// match and the game scene drives it during one, so neither has to know the
// message names.
class Session
{
public:
	enum class Phase : std::uint8_t
	{
		Offline,
		Connecting,
		Lobby,
		Waiting,
		Playing,
		Refused,
		Ended
	};

private:
	Client client;
	Lockstep lockstep;

	Phase phase = Phase::Offline;

	Vector<String> rooms;
	String notice;

	String team;
	int room = -1;

	std::uint64_t digest = 0;

	void Handle(const nlohmann::json &message);

public:
	static Session &GetInstance();

	void Connect(const String &url);
	void Disconnect();

	void RequestRooms();
	void Join(int roomId, bool asObserver);

	// Drains the socket. Called once per frame from whatever scene is up.
	void Poll();

	// An order the local player has just given. It is not applied here: the
	// server stamps it and hands it back to every client, this one included.
	void SendCommand(const Vector<int> &uids, const nlohmann::json &orders);

	void ReportDigest(std::uint64_t value);

	Lockstep &Clock()
	{
		return lockstep;
	}

	Phase GetPhase() const
	{
		return phase;
	}

	const Vector<String> &Rooms() const
	{
		return rooms;
	}

	const String &Notice() const
	{
		return notice;
	}

	const String &Team() const
	{
		return team;
	}

	int Room() const
	{
		return room;
	}

	bool IsPlaying() const
	{
		return phase == Phase::Playing;
	}
};
} // namespace TGX::Net
