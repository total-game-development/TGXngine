#include "Multiplayer.h"
#include "Enums.h"
#include "Globals.h"
#include "Logs.h"
#include "Mouse.h"
#include "Net/Session.h"
#include "Renderer.h"
#include "Window.h"

namespace TGX
{
namespace
{
constexpr float MARGIN = 60.0f;
constexpr float ROW_HEIGHT = 34.0f;
constexpr float LIST_TOP = 150.0f;
constexpr unsigned int ROW_SIZE = 16;

const sf::Color ACCENT = sf::Color(0x00, 0xA7, 0xFF);
const sf::Color MUTED = sf::Color(0x7A, 0x86, 0x96);

String ServerUrl()
{
	// Overridable so a match can be played against another machine without a
	// rebuild: set TGX_SERVER=ws://host:port.
	const char *configured = std::getenv("TGX_SERVER");

	return configured != nullptr ? String(configured) : String("ws://127.0.0.1:9001");
}
} // namespace

Multiplayer::Multiplayer()
{
	Log::Success("Multiplayer Scene Created");
}

Multiplayer::~Multiplayer()
{
	Log::Success("Deleted Multiplayer Scene");
}

void Multiplayer::Init()
{
	loaded = font.loadFromFile("Resources/courier.ttf");

	if (!loaded)
	{
		Log::Error("Multiplayer scene cannot load its font");
	}

	hovered = -1;
	scroll = 0;

	Net::Session::GetInstance().Connect(ServerUrl());
}

void Multiplayer::Update()
{
	Net::Session &session = Net::Session::GetInstance();

	session.Poll();

	Mouse &mouse = Mouse::GetInstance();

	hovered = -1;

	for (std::size_t index = 0; index < roomRows.size(); index++)
	{
		if (roomRows[index].contains(mouse.x, mouse.y))
		{
			hovered = static_cast<int>(index);
			break;
		}
	}
}

void Multiplayer::DrawHeading()
{
	Window &window = Window::GetInstance();
	Net::Session &session = Net::Session::GetInstance();

	sf::Text title("MULTIPLAYER", font, 28);
	title.setPosition(MARGIN, 50.0f);
	title.setFillColor(ACCENT);
	window.Draw(title);

	sf::Text url(ServerUrl(), font, 14);
	url.setPosition(MARGIN, 90.0f);
	url.setFillColor(MUTED);
	window.Draw(url);

	if (session.IsPlaying())
	{
		return;
	}

	sf::Text hint("Click a room to join.  TGX_SERVER overrides the address.  ESC returns.", font, 13);
	hint.setPosition(MARGIN, 112.0f);
	hint.setFillColor(MUTED);
	window.Draw(hint);
}

void Multiplayer::DrawRooms()
{
	Window &window = Window::GetInstance();
	Net::Session &session = Net::Session::GetInstance();

	roomRows.clear();

	const Vector<String> &rooms = session.Rooms();

	const float width = window.GetViewSize().x - (MARGIN * 2.0f);

	// The lobby serves more rooms than fit on screen, so only the ones with
	// space to be drawn are made clickable.
	const auto visible = static_cast<std::size_t>((window.GetViewSize().y - LIST_TOP - 80.0f) / ROW_HEIGHT);

	for (std::size_t index = 0; index < rooms.size() && index < visible; index++)
	{
		const float y = LIST_TOP + (ROW_HEIGHT * static_cast<float>(index));

		sf::FloatRect bounds(MARGIN, y, width, ROW_HEIGHT - 4.0f);

		roomRows.push_back(bounds);

		const bool isHovered = hovered == static_cast<int>(index);

		sf::RectangleShape row({bounds.width, bounds.height});
		row.setPosition(bounds.left, bounds.top);
		row.setFillColor(isHovered ? sf::Color(0x18, 0x24, 0x34) : sf::Color(0x10, 0x14, 0x1C));
		row.setOutlineThickness(1.0f);
		row.setOutlineColor(isHovered ? ACCENT : sf::Color(0x24, 0x2C, 0x38));
		window.Draw(row);

		sf::Text label(rooms[index], font, ROW_SIZE);
		label.setPosition(bounds.left + 14.0f, bounds.top + 5.0f);
		label.setFillColor(isHovered ? sf::Color::White : sf::Color(0xC8, 0xD0, 0xDC));
		window.Draw(label);
	}
}

void Multiplayer::DrawNotice()
{
	Window &window = Window::GetInstance();
	Net::Session &session = Net::Session::GetInstance();

	if (session.Notice().empty())
	{
		return;
	}

	sf::Text notice(session.Notice(), font, 15);
	notice.setPosition(MARGIN, window.GetViewSize().y - 60.0f);
	notice.setFillColor(session.GetPhase() == Net::Session::Phase::Refused ? sf::Color(0xFF, 0x6B, 0x6B) : sf::Color(0xE8, 0xE9, 0xF3));
	window.Draw(notice);
}

void Multiplayer::Draw()
{
	if (!loaded)
	{
		return;
	}

	Window &window = Window::GetInstance();

	sf::RectangleShape backdrop(window.GetViewSize());
	backdrop.setFillColor(sf::Color(0x05, 0x07, 0x0A));
	window.Draw(backdrop);

	DrawHeading();
	DrawRooms();
	DrawNotice();
}

void Multiplayer::Click()
{
	Net::Session &session = Net::Session::GetInstance();

	if (hovered < 0 || session.GetPhase() != Net::Session::Phase::Lobby)
	{
		return;
	}

	session.Join(hovered, false);
}

void Multiplayer::RightClick()
{
}

void Multiplayer::Release()
{
}

bool Multiplayer::Key(int code)
{
	if (code != static_cast<int>(sf::Keyboard::Escape))
	{
		return false;
	}

	Net::Session::GetInstance().Disconnect();

	Renderer::GetInstance().LoadScene(SceneType::Intro);

	return true;
}

void Multiplayer::Close()
{
	// The session is deliberately left connected: the lobby hands over to the
	// game scene mid-match, and disconnecting here would end it.
	roomRows.clear();
	hovered = -1;
}

void Multiplayer::Free()
{
}
} // namespace TGX
