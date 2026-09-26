#include "Multiplayer.h"
#include <cstdlib>
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
const sf::Color READY = sf::Color(0x4C, 0xD1, 0x7A);
const sf::Color ABSENT = sf::Color(0xFF, 0x9F, 0x43);

String ServerUrl()
{
	// Overridable so a match can be played against another machine without a
	// rebuild: set TGX_SERVER=ws://host:port, or wss:// for a TLS build.
	const char *configured = std::getenv("TGX_SERVER");

	return configured != nullptr ? String(configured) : String("ws://127.0.0.1:9001");
}
} // namespace

Multiplayer::Multiplayer(bool inArena) : arena(inArena)
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

	hits.clear();

	Net::Session::GetInstance().Connect(ServerUrl());
}

bool Multiplayer::InRoom() const
{
	const Net::Session::Phase phase = Net::Session::GetInstance().GetPhase();

	return phase == Net::Session::Phase::Room || phase == Net::Session::Phase::Waiting;
}

void Multiplayer::Add(const sf::FloatRect &bounds, Target target, int value)
{
	hits.push_back({bounds, target, value});
}

void Multiplayer::Update()
{
	Net::Session &session = Net::Session::GetInstance();

	session.Poll();

	Mouse &mouse = Mouse::GetInstance();

	hovered = -1;

	for (std::size_t index = 0; index < hits.size(); index++)
	{
		if (hits[index].bounds.contains(mouse.x, mouse.y))
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

	const String heading = arena ? String{"ARENA"} : String{"MULTIPLAYER"};

	sf::Text title(InRoom() && !arena ? "ROOM " + std::to_string(session.Room().number) : heading, font, 28);
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

	String hint = InRoom()
					  ? "Click a seat to move, a side to change it, a map to pick it.  ESC leaves the room."
					  : "Click a room to join, right-click to watch.  TGX_SERVER overrides the address.  ESC returns.";

	if (arena)
	{
		hint = InRoom()
				   ? "Waiting for the match.  An arena starts once two people are watching.  ESC leaves."
				   : "Click a room to watch the AI play itself there.  A match starts once two people are watching.  ESC returns.";
	}

	sf::Text line(hint, font, 13);
	line.setPosition(MARGIN, 112.0f);
	line.setFillColor(MUTED);
	window.Draw(line);
}

void Multiplayer::DrawButton(const sf::FloatRect &bounds, const String &label, bool active, bool enabled, int index)
{
	Window &window = Window::GetInstance();

	const bool isHovered = enabled && hovered == index;

	sf::RectangleShape box({bounds.width, bounds.height});
	box.setPosition(bounds.left, bounds.top);
	box.setFillColor(active ? sf::Color(0x10, 0x3A, 0x54) : (isHovered ? sf::Color(0x18, 0x24, 0x34) : sf::Color(0x10, 0x14, 0x1C)));
	box.setOutlineThickness(1.0f);
	box.setOutlineColor(active ? ACCENT : (isHovered ? ACCENT : sf::Color(0x24, 0x2C, 0x38)));
	window.Draw(box);

	sf::Text text(label, font, 14);
	text.setPosition(bounds.left + 10.0f, bounds.top + (bounds.height - 18.0f) / 2.0f);
	text.setFillColor(enabled ? (active || isHovered ? sf::Color::White : sf::Color(0xC8, 0xD0, 0xDC)) : MUTED);
	window.Draw(text);
}

void Multiplayer::DrawRooms()
{
	Window &window = Window::GetInstance();
	Net::Session &session = Net::Session::GetInstance();

	const Vector<Net::RoomSummary> &rooms = session.Rooms();

	const float width = window.GetViewSize().x - (MARGIN * 2.0f);

	// The lobby serves more rooms than fit on screen, so only the ones with
	// space to be drawn are made clickable.
	const auto visible = static_cast<std::size_t>((window.GetViewSize().y - LIST_TOP - 80.0f) / ROW_HEIGHT);

	for (std::size_t index = 0; index < rooms.size() && index < visible; index++)
	{
		const Net::RoomSummary &room = rooms[index];

		const float y = LIST_TOP + (ROW_HEIGHT * static_cast<float>(index));

		const sf::FloatRect bounds(MARGIN, y, width, ROW_HEIGHT - 4.0f);

		const int hit = static_cast<int>(hits.size());

		Add(bounds, arena ? Target::Arena : (room.running ? Target::Observe : Target::Room), static_cast<int>(index));

		const bool isHovered = hovered == hit;

		sf::RectangleShape row({bounds.width, bounds.height});
		row.setPosition(bounds.left, bounds.top);
		row.setFillColor(isHovered ? sf::Color(0x18, 0x24, 0x34) : sf::Color(0x10, 0x14, 0x1C));
		row.setOutlineThickness(1.0f);
		row.setOutlineColor(isHovered ? ACCENT : sf::Color(0x24, 0x2C, 0x38));
		window.Draw(row);

		sf::Text label(room.status, font, ROW_SIZE);
		label.setPosition(bounds.left + 14.0f, bounds.top + 5.0f);
		label.setFillColor(isHovered ? sf::Color::White : sf::Color(0xC8, 0xD0, 0xDC));
		window.Draw(label);

		// The map and the count sit on the right, so the status line on the left
		// stays readable at any width.
		sf::Text detail(
			room.levelName + "   " + std::to_string(room.occupied) + "/" + std::to_string(room.capacity),
			font, 13);
		detail.setPosition(bounds.left + bounds.width - detail.getLocalBounds().width - 14.0f, bounds.top + 7.0f);
		detail.setFillColor(room.running ? ABSENT : MUTED);
		window.Draw(detail);
	}
}

void Multiplayer::DrawRoom()
{
	Window &window = Window::GetInstance();
	Net::Session &session = Net::Session::GetInstance();

	const Net::RoomState &state = session.Room();

	float y = LIST_TOP;

	sf::Text seats("SEATS", font, 14);
	seats.setPosition(MARGIN, y);
	seats.setFillColor(MUTED);
	window.Draw(seats);

	y += 26.0f;

	const float seatWidth = 260.0f;
	const float teamWidth = 200.0f;

	for (const Net::SlotState &slot : state.slots)
	{
		const sf::FloatRect seatBounds(MARGIN, y, seatWidth, ROW_HEIGHT - 4.0f);

		String label = std::to_string(slot.index + 1) + ". ";

		if (!slot.occupied)
		{
			label += "open";
		}
		else if (slot.mine)
		{
			label += "you";
		}
		else
		{
			label += slot.connected ? "player" : "away";
		}

		const int seatHit = static_cast<int>(hits.size());

		// Only an empty seat is worth clicking; the one already sat in is not a
		// move, and somebody else's is not on offer.
		if (!slot.occupied)
		{
			Add(seatBounds, Target::Seat, slot.index);
		}

		DrawButton(seatBounds, label, slot.mine, !slot.occupied, slot.occupied ? -1 : seatHit);

		const sf::FloatRect teamBounds(MARGIN + seatWidth + 12.0f, y, teamWidth, ROW_HEIGHT - 4.0f);

		const int teamHit = static_cast<int>(hits.size());

		if (slot.mine)
		{
			Add(teamBounds, Target::Team, slot.index);
		}

		DrawButton(teamBounds, slot.team, false, slot.mine, slot.mine ? teamHit : -1);

		if (slot.occupied)
		{
			sf::Text mark(slot.connected ? (slot.ready ? "ready" : "waiting") : "reconnecting", font, 13);
			mark.setPosition(MARGIN + seatWidth + teamWidth + 26.0f, y + 7.0f);
			mark.setFillColor(!slot.connected ? ABSENT : (slot.ready ? READY : MUTED));
			window.Draw(mark);
		}

		y += ROW_HEIGHT;
	}

	y += 18.0f;

	sf::Text maps("MAP", font, 14);
	maps.setPosition(MARGIN, y);
	maps.setFillColor(MUTED);
	window.Draw(maps);

	y += 26.0f;

	const float mapWidth = 220.0f;
	const float available = window.GetViewSize().x - (MARGIN * 2.0f);
	const auto perRow = static_cast<std::size_t>(std::max(1.0f, available / (mapWidth + 10.0f)));

	for (std::size_t index = 0; index < state.levels.size(); index++)
	{
		const float column = static_cast<float>(index % perRow);
		const float row = static_cast<float>(index / perRow);

		const sf::FloatRect bounds(
			MARGIN + (column * (mapWidth + 10.0f)),
			y + (row * ROW_HEIGHT),
			mapWidth,
			ROW_HEIGHT - 4.0f);

		const int hit = static_cast<int>(hits.size());

		Add(bounds, Target::Level, static_cast<int>(index));

		DrawButton(bounds, state.levels[index], index == state.level, true, hit);
	}

	y += ROW_HEIGHT * static_cast<float>((state.levels.size() + perRow - 1) / perRow);
	y += 24.0f;

	const int mine = state.Mine();
	const bool ready = mine >= 0 && state.slots[static_cast<std::size_t>(mine)].ready;

	const sf::FloatRect readyBounds(MARGIN, y, 200.0f, 40.0f);

	const int readyHit = static_cast<int>(hits.size());

	if (mine >= 0)
	{
		Add(readyBounds, Target::Ready, ready ? 0 : 1);
	}

	DrawButton(readyBounds, ready ? "READY" : "NOT READY", ready, mine >= 0, mine >= 0 ? readyHit : -1);

	const sf::FloatRect backBounds(MARGIN + 212.0f, y, 140.0f, 40.0f);

	const int backHit = static_cast<int>(hits.size());

	Add(backBounds, Target::Back, 0);

	DrawButton(backBounds, "LEAVE", false, true, backHit);

	if (state.observers > 0)
	{
		sf::Text watching(std::to_string(state.observers) + " watching", font, 13);
		watching.setPosition(MARGIN + 364.0f, y + 13.0f);
		watching.setFillColor(MUTED);
		window.Draw(watching);
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

	const Net::Session::Phase phase = session.GetPhase();

	sf::Color colour = sf::Color(0xE8, 0xE9, 0xF3);

	if (phase == Net::Session::Phase::Refused)
	{
		colour = sf::Color(0xFF, 0x6B, 0x6B);
	}
	else if (phase == Net::Session::Phase::Resuming)
	{
		colour = ABSENT;
	}

	sf::Text notice(session.Notice(), font, 15);
	notice.setPosition(MARGIN, window.GetViewSize().y - 60.0f);
	notice.setFillColor(colour);
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

	// Collected fresh every frame, because the controls move as seats fill and
	// the map list wraps. A stale hit region would act on the wrong thing.
	hits.clear();

	DrawHeading();

	// An arena between matches has no seats to show; the notice says it is
	// waiting for the next.
	if (InRoom() && arena)
	{
		DrawNotice();
	}
	else if (InRoom())
	{
		DrawRoom();
	}
	else
	{
		DrawRooms();
	}

	DrawNotice();
}

void Multiplayer::Click()
{
	Net::Session &session = Net::Session::GetInstance();

	if (hovered < 0 || hovered >= static_cast<int>(hits.size()))
	{
		return;
	}

	const Hit hit = hits[static_cast<std::size_t>(hovered)];

	switch (hit.target)
	{
		case Target::Room:
			session.Join(hit.value, false);
			break;

		case Target::Observe:
			// A room that is playing cannot take another player, but it can take
			// somebody to watch: the server replays the match into them.
			session.Join(hit.value, true);
			break;

		case Target::Arena:
			session.JoinArena(hit.value);
			break;

		case Target::Seat:
			session.ChooseSlot(hit.value);
			break;

		case Target::Team:
			{
				const Net::RoomState &state = session.Room();

				if (state.teams.empty())
				{
					break;
				}

				// Cycles through the sides rather than opening a list: the server
				// refuses one already taken, so a second click moves on again.
				const String current = state.slots[static_cast<std::size_t>(hit.value)].team;

				std::size_t next = 0;

				for (std::size_t index = 0; index < state.teams.size(); index++)
				{
					if (state.teams[index] == current)
					{
						next = (index + 1) % state.teams.size();
						break;
					}
				}

				session.ChooseTeam(state.teams[next]);
				break;
			}

		case Target::Level:
			session.ChooseLevel(static_cast<std::size_t>(hit.value));
			break;

		case Target::Ready:
			session.SetReady(hit.value != 0);
			break;

		case Target::Back:
			session.Leave();
			break;

		default:
			break;
	}
}

void Multiplayer::RightClick()
{
	Net::Session &session = Net::Session::GetInstance();

	if (InRoom() || hovered < 0 || hovered >= static_cast<int>(hits.size()))
	{
		return;
	}

	const Hit hit = hits[static_cast<std::size_t>(hovered)];

	if (hit.target == Target::Arena)
	{
		session.JoinArena(hit.value);
	}
	else if (hit.target == Target::Room || hit.target == Target::Observe)
	{
		session.Join(hit.value, true);
	}
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

	Net::Session &session = Net::Session::GetInstance();

	// Inside a room, escape gives the seat up and goes back to the list. On the
	// list it closes the session and leaves the lobby.
	if (InRoom())
	{
		session.Leave();
		return true;
	}

	session.Disconnect();

	Renderer::GetInstance().LoadScene(SceneType::Intro);

	return true;
}

void Multiplayer::Close()
{
	// The session is deliberately left connected: the lobby hands over to the
	// game scene mid-match, and disconnecting here would end it.
	hits.clear();
	hovered = -1;
}

void Multiplayer::Free()
{
}
} // namespace TGX
