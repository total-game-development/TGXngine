#pragma once

#include <ixwebsocket/IXWebSocket.h>
#include <mutex>
#include <nlohmann/json.hpp>
#include "Core.h"

namespace TGX::Net
{
enum class Status : std::uint8_t
{
	Idle,
	Connecting,
	Connected,
	Closed,
	Failed
};

// The socket runs on its own thread, so nothing it delivers is touched where it
// arrives: messages are queued and drained from the scene's update, keeping the
// whole game on one thread as before.
class Client
{
private:
	ix::WebSocket socket;

	Vector<nlohmann::json> inbound;
	mutable std::mutex mutex;

	Status status = Status::Idle;
	String failure;

public:
	Client() = default;
	~Client();

	Client(const Client &) = delete;
	Client &operator=(const Client &) = delete;

	void Connect(const String &url);
	void Disconnect();

	void Send(const nlohmann::json &message);

	// Everything received since the last call, in arrival order.
	Vector<nlohmann::json> Drain();

	Status GetStatus() const;
	String Failure() const;

	bool IsConnected() const
	{
		return GetStatus() == Status::Connected;
	}
};
} // namespace TGX::Net
