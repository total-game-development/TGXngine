#include "Client.h"
#include <ixwebsocket/IXNetSystem.h>
#include <cstdlib>
#include "Logs.h"

namespace TGX::Net
{
namespace
{
// Winsock has to be up before any socket call, and only once per process.
void EnsureNetSystem()
{
	static bool started = false;

	if (!started)
	{
		ix::initNetSystem();
		started = true;
	}
}

#ifdef IXWEBSOCKET_USE_TLS
// Where to look for the authority that signs the server's certificate.
// SYSTEM uses the platform's own store; TGX_SERVER_CA points at a file, which is
// what a self-signed certificate on a development machine needs.
String CertificateAuthority()
{
	const char *configured = std::getenv("TGX_SERVER_CA");

	return configured != nullptr ? String(configured) : String("SYSTEM");
}
#endif
} // namespace

Client::~Client()
{
	Disconnect();
}

void Client::Connect(const String &url)
{
	EnsureNetSystem();

	Disconnect();

	{
		std::lock_guard<std::mutex> guard(mutex);

		inbound.clear();
		status = Status::Connecting;
		failure.clear();
	}

	// wss:// needs a build that has TLS compiled in. IXWebSocket does not say so
	// itself -- it simply never connects -- so the refusal is spelled out here.
	if (url.rfind("wss://", 0) == 0)
	{
#ifdef IXWEBSOCKET_USE_TLS
		ix::SocketTLSOptions tls;

		tls.tls = true;
		tls.caFile = CertificateAuthority();

		socket.setTLSOptions(tls);
#else
		std::lock_guard<std::mutex> guard(mutex);

		status = Status::Failed;
		failure = "this build has no TLS; configure with -DTGX_ENABLE_TLS=ON or use a ws:// address";

		Log::Error("Cannot connect to " + url + ": " + failure);

		return;
#endif
	}

	socket.setUrl(url);
	socket.disableAutomaticReconnection();

	socket.setOnMessageCallback([this](const ix::WebSocketMessagePtr &message) {
		std::lock_guard<std::mutex> guard(mutex);

		switch (message->type)
		{
			case ix::WebSocketMessageType::Open:
				status = Status::Connected;
				break;

			case ix::WebSocketMessageType::Close:
				status = Status::Closed;
				break;

			case ix::WebSocketMessageType::Error:
				status = Status::Failed;
				failure = message->errorInfo.reason;
				break;

			case ix::WebSocketMessageType::Message:
				try
				{
					inbound.push_back(nlohmann::json::parse(message->str));
				}
				catch (const nlohmann::json::exception &)
				{
					// A malformed frame is dropped rather than taken as a
					// disconnect; the match carries on without it.
				}
				break;

			default:
				break;
		}
	});

	socket.start();

	Log::Info("Connecting to " + url);
}

void Client::Disconnect()
{
	if (status == Status::Idle)
	{
		return;
	}

	// The close code is spelled out rather than defaulted: IXWebSocket is built
	// here as a DLL, and its WebSocketCloseConstants are static const members
	// that a DLL does not export, so the defaults do not link.
	socket.stop(1000, "closing");

	std::lock_guard<std::mutex> guard(mutex);

	status = Status::Idle;
	inbound.clear();
}

void Client::Send(const nlohmann::json &message)
{
	if (!IsConnected())
	{
		return;
	}

	socket.sendText(message.dump());
}

Vector<nlohmann::json> Client::Drain()
{
	std::lock_guard<std::mutex> guard(mutex);

	Vector<nlohmann::json> messages;
	messages.swap(inbound);

	return messages;
}

Status Client::GetStatus() const
{
	std::lock_guard<std::mutex> guard(mutex);

	return status;
}

String Client::Failure() const
{
	std::lock_guard<std::mutex> guard(mutex);

	return failure;
}
} // namespace TGX::Net
