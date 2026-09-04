#include "Skirmish.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include "Globals.h"
#include "Window.h"
#include "WorldState.h"

namespace TGX
{
namespace
{
const Vector<String> &Roles()
{
	static const Vector<String> roles = {"player", "ai", "none"};
	return roles;
}

constexpr unsigned int titleSize = 34;
constexpr unsigned int headingSize = 20;
constexpr unsigned int bodySize = 16;

constexpr float panelX = 40.0f;
constexpr float panelY = 90.0f;
constexpr float rowHeight = 34.0f;
constexpr float rowGap = 8.0f;
constexpr float teamCellWidth = 180.0f;
constexpr float roleCellWidth = 120.0f;

const sf::Color backdrop(8, 10, 14, 255);
const sf::Color panelFill(18, 22, 30, 235);
const sf::Color cellFill(34, 42, 56, 255);
const sf::Color cellHover(58, 72, 96, 255);
const sf::Color cellEdge(96, 116, 148, 255);
const sf::Color accent(255, 215, 0, 255);
const sf::Color muted(150, 160, 175, 255);

String Titled(const String &value)
{
	if (value == "ai")
	{
		return "AI";
	}

	String result = value;
	bool atStart = true;

	for (char &character : result)
	{
		if (atStart && character >= 'a' && character <= 'z')
		{
			character = static_cast<char>(character - ('a' - 'A'));
		}

		atStart = (character == '-' || character == ' ' || character == '_');
	}

	return result;
}
} // namespace

Skirmish::Skirmish()
{
	Log::Success("Skirmish Created");
}

Skirmish::~Skirmish()
{
	Log::Success("Deleted Skirmish");
}

void Skirmish::Init()
{
	const String path = "Resources/maps.json";

	if (!std::filesystem::exists(path))
	{
		Log::Error("Skirmish: maps file missing: " + path);
		return;
	}

	std::ifstream stream(path);

	if (!(stream >> maps))
	{
		Log::Error("Skirmish: failed to parse " + path);
		return;
	}

	if (!font.loadFromFile("Resources/courier.ttf"))
	{
		Log::Error("Skirmish: cannot load Resources/courier.ttf");
	}

	WorldState &world = WorldState::GetInstance();

	// Laid out in the 1040x720 design canvas and centred in production, which is
	// what Intro does with its buttons.
	originX = world.IsProduction() ? (static_cast<float>(world.GetCanvasOffsetWidth()) / 2.0f) : 0.0f;
	originY = world.IsProduction() ? (static_cast<float>(world.GetCanvasOffsetHeight()) / 2.0f) : 0.0f;

	CollectLevels();

	if (levels.empty())
	{
		Log::Warning("Skirmish: no map in maps.json carries \"type\":\"skirmish\"");
		return;
	}

	SelectLevel(0);

	Log::Success("Skirmish Init: " + std::to_string(levels.size()) + " map(s)");
}

void Skirmish::CollectLevels()
{
	levels.clear();

	if (!maps.contains("singleplayer") || !maps["singleplayer"].is_array())
	{
		return;
	}

	const json &all = maps["singleplayer"];

	for (std::size_t i = 0; i < all.size(); i++)
	{
		if (all[i].value("type", String{}) == "skirmish")
		{
			levels.push_back(static_cast<int>(i));
		}
	}
}

const json &Skirmish::CurrentLevel() const
{
	return maps["singleplayer"][static_cast<std::size_t>(levels[cursor])];
}

String Skirmish::CurrentBriefing() const
{
	return CurrentLevel().value("briefing", String{"No briefing for this map."});
}

void Skirmish::SelectLevel(std::size_t index)
{
	cursor = index;

	rows.clear();
	teamChoices.clear();

	const json &level = CurrentLevel();

	if (level.contains("teams") && level["teams"].is_array())
	{
		for (const auto &team : level["teams"])
		{
			Row row;
			row.team = team.value("name", String{});
			row.role = team.value("type", String{"none"});

			rows.push_back(row);
			teamChoices.push_back(row.team);
		}
	}

	LoadPreview();
	Layout();
}

void Skirmish::LoadPreview()
{
	hasPreview = false;

	String file = CurrentLevel().value("mapPreview", String{});

	if (file.empty())
	{
		const String mapImage = CurrentLevel().value("mapImage", String{});

		if (mapImage.empty())
		{
			return;
		}

		file = mapImage + "0.png";
	}

	if (!std::filesystem::exists(file) || !previewTexture.loadFromFile(file))
	{
		return;
	}

	preview.setTexture(previewTexture, true);

	const float panelWidth = 300.0f;
	const float panelHeight = 200.0f;
	const auto textureWidth = static_cast<float>(previewTexture.getSize().x);
	const auto textureHeight = static_cast<float>(previewTexture.getSize().y);

	if (textureWidth <= 0.0f || textureHeight <= 0.0f)
	{
		return;
	}

	const float scale = std::min(panelWidth / textureWidth, panelHeight / textureHeight);

	preview.setScale(scale, scale);
	preview.setPosition(
		originX + 660.0f + (panelWidth - textureWidth * scale) / 2.0f,
		originY + panelY + 34.0f + (panelHeight - textureHeight * scale) / 2.0f);

	hasPreview = true;
}

// Built once and read by both Draw and Click, so the picture and the hit test
// cannot drift apart.
void Skirmish::Layout()
{
	hotspots.clear();

	const float left = originX + panelX;
	const float top = originY + panelY;

	hotspots.push_back({Hotspot::Kind::PreviousMap, 0, left, top + 34.0f, 34.0f, 30.0f});
	hotspots.push_back({Hotspot::Kind::NextMap, 0, left + 260.0f, top + 34.0f, 34.0f, 30.0f});

	const float rowsTop = top + 110.0f;

	for (std::size_t i = 0; i < rows.size(); i++)
	{
		const float y = rowsTop + static_cast<float>(i) * (rowHeight + rowGap);

		hotspots.push_back({Hotspot::Kind::CycleTeam, i, left, y, teamCellWidth, rowHeight});
		hotspots.push_back({Hotspot::Kind::CycleRole, i, left + teamCellWidth + 10.0f, y, roleCellWidth, rowHeight});
	}

	hotspots.push_back({Hotspot::Kind::Cancel, 0, originX + 40.0f, originY + 640.0f, 130.0f, 40.0f});
	hotspots.push_back({Hotspot::Kind::Start, 0, originX + 870.0f, originY + 640.0f, 130.0f, 40.0f});
}

void Skirmish::CycleTeam(std::size_t row)
{
	if (row >= rows.size() || teamChoices.empty())
	{
		return;
	}

	auto found = std::find(teamChoices.begin(), teamChoices.end(), rows[row].team);
	std::size_t next = 0;

	if (found != teamChoices.end())
	{
		next = (static_cast<std::size_t>(std::distance(teamChoices.begin(), found)) + 1) % teamChoices.size();
	}

	rows[row].team = teamChoices[next];
}

void Skirmish::CycleRole(std::size_t row)
{
	if (row >= rows.size())
	{
		return;
	}

	const Vector<String> &roles = Roles();

	auto found = std::find(roles.begin(), roles.end(), rows[row].role);
	std::size_t next = 0;

	if (found != roles.end())
	{
		next = (static_cast<std::size_t>(std::distance(roles.begin(), found)) + 1) % roles.size();
	}

	rows[row].role = roles[next];

	// One human only: the camera looks through a single team and the interface
	// spends a single purse.
	if (rows[row].role == "player")
	{
		for (std::size_t i = 0; i < rows.size(); i++)
		{
			if (i != row && rows[i].role == "player")
			{
				rows[i].role = "ai";
			}
		}
	}
}

void Skirmish::Commit()
{
	SkirmishSetup::Clear();

	SkirmishSetup::active = true;
	SkirmishSetup::level = levels[cursor];

	for (const Row &row : rows)
	{
		SkirmishSetup::slots.push_back({row.team, row.role});
	}

	SkirmishSetup::spectator = SkirmishSetup::PlayerTeam().empty();

	Log::Success("Skirmish starting map " + std::to_string(SkirmishSetup::level) +
				 (SkirmishSetup::spectator ? " (spectated)" : " as " + SkirmishSetup::PlayerTeam()));
}

void Skirmish::DrawPanel(float x, float y, float w, float h, sf::Color fill, sf::Color edge)
{
	sf::RectangleShape shape({w, h});
	shape.setPosition(x, y);
	shape.setFillColor(fill);
	shape.setOutlineColor(edge);
	shape.setOutlineThickness(1.0f);

	Window::GetInstance().Draw(shape);
}

void Skirmish::DrawLabel(const String &text, float x, float y, unsigned int size, sf::Color colour)
{
	sf::Text drawText(text, font, size);
	drawText.setPosition(x, y);
	drawText.setFillColor(colour);

	Window::GetInstance().Draw(drawText);
}

float Skirmish::MeasureLabel(const String &text, unsigned int size)
{
	sf::Text drawText(text, font, size);
	return drawText.getLocalBounds().width;
}

void Skirmish::Update()
{
}

void Skirmish::Draw()
{
	WorldState &world = WorldState::GetInstance();

	const float screenWidth = static_cast<float>(Globals::canvasWidth + world.GetCanvasOffsetWidth());
	const float screenHeight = static_cast<float>(Globals::canvasHeight + world.GetCanvasOffsetHeight());

	DrawPanel(0.0f, 0.0f, screenWidth, screenHeight, backdrop, backdrop);

	DrawLabel("SKIRMISH", originX + panelX, originY + 34.0f, titleSize, accent);

	if (levels.empty())
	{
		DrawLabel("No skirmish maps found in maps.json.", originX + panelX, originY + panelY, headingSize, muted);
		return;
	}

	const float left = originX + panelX;
	const float top = originY + panelY;

	DrawPanel(left - 12.0f, top - 12.0f, 600.0f, 500.0f, panelFill, cellEdge);

	DrawLabel("MAP", left, top, headingSize, muted);

	const float mouseX = world.GetMouseX();
	const float mouseY = world.GetMouseY();

	for (const Hotspot &spot : hotspots)
	{
		String label;
		sf::Color colour = sf::Color::White;

		switch (spot.kind)
		{
			case Hotspot::Kind::PreviousMap:
				label = "<";
				break;

			case Hotspot::Kind::NextMap:
				label = ">";
				break;

			case Hotspot::Kind::CycleTeam:
				label = Titled(rows[spot.row].team);
				break;

			case Hotspot::Kind::CycleRole:
				label = Titled(rows[spot.row].role);
				colour = (rows[spot.row].role == "none") ? muted : accent;
				break;

			case Hotspot::Kind::Cancel:
				label = "Cancel";
				break;

			case Hotspot::Kind::Start:
				label = "Start";
				colour = accent;
				break;
		}

		const bool hovered = spot.Contains(mouseX, mouseY);

		DrawPanel(spot.x, spot.y, spot.w, spot.h, hovered ? cellHover : cellFill, cellEdge);

		DrawLabel(label,
				  spot.x + (spot.w - MeasureLabel(label, bodySize)) / 2.0f,
				  spot.y + (spot.h - static_cast<float>(bodySize)) / 2.0f - 3.0f,
				  bodySize, colour);
	}

	const json &level = CurrentLevel();

	DrawLabel(Titled(level.value("name", String{"unnamed"})), left + 44.0f, top + 40.0f, headingSize, sf::Color::White);

	DrawLabel(std::to_string(cursor + 1) + " / " + std::to_string(levels.size()),
			  left + 44.0f, top + 66.0f, bodySize, muted);

	DrawLabel("TEAMS", left, top + 88.0f, headingSize, muted);

	const float briefingTop = top + 110.0f + static_cast<float>(rows.size()) * (rowHeight + rowGap) + 24.0f;

	DrawLabel("BRIEFING", left, briefingTop, headingSize, muted);

	// Wrapped against the panel, which is narrower than any of the briefings.
	const String briefing = CurrentBriefing();

	Vector<String> lines;
	String line;
	std::size_t start = 0;

	while (start <= briefing.size())
	{
		const std::size_t breakAt = briefing.find(' ', start);
		const String word = briefing.substr(start, (breakAt == String::npos) ? String::npos : breakAt - start);
		const String candidate = line.empty() ? word : (line + " " + word);

		if (!line.empty() && MeasureLabel(candidate, bodySize) > 560.0f)
		{
			lines.push_back(line);
			line = word;
		}
		else
		{
			line = candidate;
		}

		if (breakAt == String::npos)
		{
			break;
		}

		start = breakAt + 1;
	}

	if (!line.empty())
	{
		lines.push_back(line);
	}

	for (std::size_t i = 0; i < lines.size() && i < 12; i++)
	{
		DrawLabel(lines[i], left, briefingTop + 28.0f + static_cast<float>(i) * 20.0f, bodySize, sf::Color::White);
	}

	DrawPanel(originX + 660.0f, originY + panelY + 34.0f, 300.0f, 200.0f, sf::Color(12, 14, 20, 255), cellEdge);

	DrawLabel("PREVIEW", originX + 660.0f, originY + panelY, headingSize, muted);

	if (hasPreview)
	{
		Window::GetInstance().Draw(preview);
	}
	else
	{
		DrawLabel("no preview", originX + 700.0f, originY + panelY + 128.0f, bodySize, muted);
	}

	DrawLabel(std::to_string(level.value("mapGridWidth", 0)) + " x " + std::to_string(level.value("mapGridHeight", 0)) + " cells",
			  originX + 660.0f, originY + panelY + 248.0f, bodySize, muted);

	bool anyPlayer = false;

	for (const Row &row : rows)
	{
		anyPlayer = anyPlayer || row.role == "player";
	}

	if (!anyPlayer && !rows.empty())
	{
		DrawLabel("No human side: the match will be spectated.",
				  originX + 660.0f, originY + panelY + 272.0f, bodySize, accent);
	}
}

void Skirmish::Click()
{
	if (levels.empty())
	{
		return;
	}

	WorldState &world = WorldState::GetInstance();

	const float mouseX = world.GetMouseX();
	const float mouseY = world.GetMouseY();

	for (const Hotspot &spot : hotspots)
	{
		if (!spot.Contains(mouseX, mouseY))
		{
			continue;
		}

		switch (spot.kind)
		{
			case Hotspot::Kind::PreviousMap:
				SelectLevel((cursor == 0) ? levels.size() - 1 : cursor - 1);
				return;

			case Hotspot::Kind::NextMap:
				SelectLevel((cursor + 1) % levels.size());
				return;

			case Hotspot::Kind::CycleTeam:
				CycleTeam(spot.row);
				return;

			case Hotspot::Kind::CycleRole:
				CycleRole(spot.row);
				return;

			case Hotspot::Kind::Start:
				Commit();

				// Queued: loading a scene frees the one asking for it, and this
				// is running inside a method of that scene.
				world.gameEvents.emplace_back(UIAction::LoadScene, "game");
				return;

			case Hotspot::Kind::Cancel:
				SkirmishSetup::Clear();
				world.gameEvents.emplace_back(UIAction::LoadScene, "intro");
				return;
		}
	}
}

void Skirmish::RightClick()
{
}

void Skirmish::Release()
{
}

void Skirmish::Close()
{
	Log::Success("Close Skirmish");
}

void Skirmish::Free()
{
	// SkirmishSetup survives: Free runs as this screen hands over to the game,
	// and what it agreed is what the game is about to read. Cancel clears it.
	hotspots.clear();
	rows.clear();
	teamChoices.clear();
	levels.clear();
	maps = json{};
	hasPreview = false;
}
} // namespace TGX
