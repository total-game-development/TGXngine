#pragma once

#include "Client.h"
#include "Lockstep.h"

namespace TGX::Net
{
// One room as the lobby lists it.
struct RoomSummary
{
	int number = 0;
	String status;
	String levelName;

	int occupied = 0;
	int capacity = 0;
	std::size_t level = 0;

	bool running = false;
};

// One seat in the room a player has joined.
struct SlotState
{
	int index = 0;
	String team;

	bool occupied = false;
	bool connected = false;
	bool ready = false;
	bool mine = false;
};

// The room as it stands, once joined: who is sitting where, on which map, and
// whether everybody has said they are ready.
struct RoomState
{
	int number = 0;
	std::size_t level = 0;
	String levelName;
	String status;

	Vector<String> levels;
	Vector<String> teams;
	Vector<SlotState> slots;

	int observers = 0;

	bool running = false;
	bool canStart = false;

	int Mine() const
	{
		for (const SlotState &slot : slots)
		{
			if (slot.mine)
			{
				return slot.index;
			}
		}

		return -1;
	}
};

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
		Room,
		Waiting,
		Playing,
		Resuming,
		Refused,
		Ended
	};

private:
	// How many times a match that has lost its socket tries to get it back
	// before giving up. The server holds the seat for its own grace period; this
	// only has to outlast a network blip, not a reboot.
	static constexpr int RECONNECT_ATTEMPTS = 20;

	// Frames between tries. Poll runs once a frame, so without a wait the whole
	// budget would be spent in the third of a second after the server went away.
	static constexpr int RECONNECT_COOLDOWN = 90;

	Client client;
	Lockstep lockstep;

	Phase phase = Phase::Offline;

	// What the phase was before the socket dropped, so a failed resume returns
	// to the lobby rather than to a match that is no longer there.
	Phase interrupted = Phase::Offline;

	Vector<RoomSummary> rooms;
	Vector<String> levels;

	RoomState state;

	String notice;
	String url;

	String team;
	int room = -1;
	int slot = -1;

	// Issued by the server when the seat was taken, presented to get it back.
	String token;

	int attempts = 0;
	int cooldown = 0;
	bool observer = false;
	bool held = false;

	// Set when a resume arrives, cleared once the game scene has taken them.
	Vector<nlohmann::json> replay;

	// What other players' consoles have sent this one, held until the game
	// scene hands it to the shell.
	Vector<nlohmann::json> shell;

	void Handle(const nlohmann::json &message);
	void EnterMatch(const nlohmann::json &message, bool resuming);
	void Rejoin();

	// True when the drop is worth another try and the wait between tries has
	// passed. Moves the session into Resuming and reopens the socket.
	bool Retry();

public:
	static Session &GetInstance();

	void Connect(const String &url);
	void Disconnect();

	void RequestRooms();
	void Join(int roomId, bool asObserver);

	// Joins as the room's headless host, with the token the server started it
	// with. It is handed the match as an observer is, and sits in no seat.
	void JoinAsHost(int roomId, const String &hostToken);

	// The host's verdict on a match: which side is left standing.
	void ReportOutcome(const String &outcome);
	void Leave();

	void ChooseSlot(int index);
	void ChooseTeam(const String &name);
	void ChooseLevel(std::size_t index);
	void SetReady(bool ready);

	// Drains the socket. Called once per frame from whatever scene is up.
	void Poll();

	// An order the local player has just given. It is not applied here: the
	// server stamps it and hands it back to every client, this one included.
	void SendCommand(const Vector<int> &uids, const nlohmann::json &orders);

	// Two folds, checked against different things. The world fold is held
	// against the other clients', since the server has no world to hold it to.
	// The command fold is held against the server's own, which it can produce
	// without a world: it folds the commands it stamped at the ticks it stamped
	// them for, so a client that applied a different set is caught outright.
	void ReportDigest(std::uint64_t world, std::uint64_t commands);

	// The commands a client that joined part-way through has to replay to reach
	// the tick the rest of the room is on. Taken once, by the game scene, as it
	// starts the match.
	Vector<nlohmann::json> TakeReplay();

	// Console traffic between players, relayed by the server to the player on
	// the named side. It never touches the simulation, so it rides beside the
	// command path rather than on it: nothing here is stamped or folded.
	void SendShell(const String &to, const nlohmann::json &body);
	Vector<nlohmann::json> TakeShell();

	Lockstep &Clock()
	{
		return lockstep;
	}

	Phase GetPhase() const
	{
		return phase;
	}

	const Vector<RoomSummary> &Rooms() const
	{
		return rooms;
	}

	const Vector<String> &Levels() const
	{
		return levels;
	}

	const RoomState &Room() const
	{
		return state;
	}

	const String &Notice() const
	{
		return notice;
	}

	const String &Team() const
	{
		return team;
	}

	int RoomId() const
	{
		return room;
	}

	int Slot() const
	{
		return slot;
	}

	bool IsObserver() const
	{
		return observer;
	}

	// True while the server has stopped the clock because somebody who belongs
	// in the match is not connected.
	bool IsHeld() const
	{
		return held;
	}

	bool IsPlaying() const
	{
		return phase == Phase::Playing;
	}

	// True while the socket is being got back, which is not the same as being
	// offline: the seat is still held and the match is still there.
	bool IsResuming() const
	{
		return phase == Phase::Resuming;
	}
};
} // namespace TGX::Net
