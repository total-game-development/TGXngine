#include "Minimap.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include "Globals.h"
#include "Window.h"
#include "WorldState.h"

namespace TGX
{
namespace
{
constexpr float SWEEP_SECONDS = 0.75f;
constexpr float DRAG_THRESHOLD = 3.0f;
constexpr float MAX_PING_RADIUS = 12.0f;

const sf::Color OWN(255, 255, 0);
const sf::Color SELECTED(255, 255, 255);
const sf::Color ENEMY(255, 0, 0);
const sf::Color RESOURCE(0, 200, 255);
const sf::Color VIEW(255, 255, 255, 200);
const sf::Color SWEEP(0, 255, 120);
const sf::Color PING(0, 255, 255);
const sf::Color NO_POWER(255, 80, 80);

class Layer : public sf::Drawable
{
public:
	sf::VertexArray vertices{sf::Quads, 4};
	const sf::Texture *texture = nullptr;

private:
	void draw(sf::RenderTarget &target, sf::RenderStates states) const override
	{
		states.texture = texture;
		target.draw(vertices, states);
	}
};

float ViewWidth(const WorldState &world)
{
	return static_cast<float>(world.GetCanvasWidth()) - world.GetBackgroundOffsetWidth() + static_cast<float>(world.GetCanvasOffsetWidth());
}

float ViewHeight(const WorldState &world)
{
	return static_cast<float>(world.GetCanvasHeight()) - world.GetBackgroundOffsetY() + static_cast<float>(world.GetCanvasOffsetHeight());
}

float CameraLeft(const WorldState &world)
{
	return world.GetBackgroundOffsetX() - world.GetMapXOffset();
}

float CameraTop(const WorldState &world)
{
	return world.GetBackgroundOffsetY() - world.GetMapYOffset();
}
} // namespace

Minimap::Minimap()
	: marks(sf::Quads)
{
}

void Minimap::Load(const String &name)
{
	const String path = "Resources/" + name + ".json";

	if (!std::filesystem::exists(path))
	{
		return;
	}

	nlohmann::json config;
	std::ifstream stream(path);

	if (!(stream >> config) || !config.contains("sidebar") || !config["sidebar"].contains("minimap"))
	{
		Log::Warning("No minimap in " + path);
		return;
	}

	const nlohmann::json &minimap = config["sidebar"]["minimap"];
	const WorldState &world = WorldState::GetInstance();

	bounds = sf::FloatRect(
		minimap.value("x", 902.0f) + static_cast<float>(world.GetCanvasOffsetWidth()),
		minimap.value("y", 17.0f),
		minimap.value("width", 120.0f),
		minimap.value("height", 122.0f));

	configured = true;

	Reset();
}

void Minimap::Reset()
{
	gridWidth = 0;
	gridHeight = 0;
	zoomOut = false;
	terrainReady = false;
	shadeRevision = -1;
	shaded = false;
	operating = false;
	standing = false;
	sweeping = false;
	holding = false;
	dragging = false;
}

void Minimap::Prepare()
{
	WorldState &world = WorldState::GetInstance();

	const int width = world.GetMapGridWidth();
	const int height = world.GetMapGridHeight();

	if (width != gridWidth || height != gridHeight)
	{
		gridWidth = width;
		gridHeight = height;
		terrainReady = false;
		shadeRevision = -1;
	}

	if (gridWidth <= 0 || gridHeight <= 0)
	{
		return;
	}

	const size_t cells = static_cast<size_t>(gridWidth) * gridHeight;

	if (!terrainReady && world.terrainPixels.size() == cells * 4)
	{
		terrainTexture.create(static_cast<unsigned int>(gridWidth), static_cast<unsigned int>(gridHeight));
		terrainTexture.update(world.terrainPixels.data());
		terrainTexture.setSmooth(true);
		terrainReady = true;
	}

	shaded = world.IsFogOfWarEnabled() && world.sightGrid.size() == cells;

	if (shaded && shadeRevision != world.sightRevision)
	{
		if (shadeTexture.getSize() != sf::Vector2u(static_cast<unsigned int>(gridWidth), static_cast<unsigned int>(gridHeight)))
		{
			shadeTexture.create(static_cast<unsigned int>(gridWidth), static_cast<unsigned int>(gridHeight));
			shadeTexture.setSmooth(true);
		}

		shadePixels.assign(cells * 4, 0);

		for (size_t cell = 0; cell < cells; cell++)
		{
			const std::uint8_t sight = world.sightGrid[cell];

			shadePixels[(cell * 4) + 3] = sight >= 2 ? 0 : (sight == 1 ? 140 : 255);
		}

		shadeTexture.update(shadePixels.data());
		shadeRevision = world.sightRevision;
	}

	const String team = world.GetTeam();
	const bool wasOperating = operating;

	standing = world.HasStanding(team, "radar");
	operating = world.IsOperating(team, "radar");

	if (operating && !wasOperating)
	{
		sweeping = true;
		sweepClock.restart();
	}
}

void Minimap::Frame()
{
	const WorldState &world = WorldState::GetInstance();

	const float width = static_cast<float>(gridWidth);
	const float height = static_cast<float>(gridHeight);
	const float cell = static_cast<float>(Globals::grid_size);

	if (zoomOut)
	{
		scale = std::min(bounds.width / width, bounds.height / height);
		area = sf::FloatRect(0.0f, 0.0f, width, height);
	}
	else
	{
		const float areaWidth = std::min(width, ViewWidth(world) / cell);
		scale = bounds.width / areaWidth;
		const float areaHeight = std::min(height, bounds.height / scale);

		const float centreX = (CameraLeft(world) + (ViewWidth(world) / 2.0f)) / cell;
		const float centreY = (CameraTop(world) + (ViewHeight(world) / 2.0f)) / cell;

		area = sf::FloatRect(
			std::clamp(centreX - (areaWidth / 2.0f), 0.0f, width - areaWidth),
			std::clamp(centreY - (areaHeight / 2.0f), 0.0f, height - areaHeight),
			areaWidth,
			areaHeight);
	}

	origin = sf::Vector2f(
		std::floor(bounds.left + ((bounds.width - (area.width * scale)) / 2.0f)),
		std::floor(bounds.top + ((bounds.height - (area.height * scale)) / 2.0f)));
}

void Minimap::Track()
{
	WorldState &world = WorldState::GetInstance();

	const float mouseX = world.GetMouseX();
	const float mouseY = world.GetMouseY();

	if (holding)
	{
		if (world.IsPointerCaptured())
		{
			if (operating && zoomOut)
			{
				if (!dragging && (std::abs(mouseX - pressed.x) > DRAG_THRESHOLD || std::abs(mouseY - pressed.y) > DRAG_THRESHOLD))
				{
					dragging = true;
				}

				if (dragging)
				{
					const sf::Vector2f cell = ToCell(
						std::clamp(mouseX, origin.x, origin.x + (area.width * scale)),
						std::clamp(mouseY, origin.y, origin.y + (area.height * scale)));

					world.RequestCamera(cell.x, cell.y);
				}
			}
		}
		else
		{
			if (operating && !dragging && bounds.contains(mouseX, mouseY))
			{
				zoomOut = !zoomOut;
			}

			holding = false;
			dragging = false;
		}
	}

	if (operating && OverMap(mouseX, mouseY))
	{
		const sf::Vector2f cell = ToCell(mouseX, mouseY);

		world.SetMinimapPoint(true, cell.x, cell.y);
	}
	else
	{
		world.SetMinimapPoint(false);
	}
}

bool Minimap::Click()
{
	if (!configured)
	{
		return false;
	}

	WorldState &world = WorldState::GetInstance();

	const float mouseX = world.GetMouseX();
	const float mouseY = world.GetMouseY();

	if (!bounds.contains(mouseX, mouseY))
	{
		return false;
	}

	world.SetPointerCaptured(true);
	world.SetSkipSelectionRemoval(true);

	holding = true;
	dragging = false;
	pressed = sf::Vector2f(mouseX, mouseY);

	return true;
}

void Minimap::Draw()
{
	if (!configured)
	{
		return;
	}

	Prepare();

	if (gridWidth <= 0 || gridHeight <= 0)
	{
		return;
	}

	Frame();
	Track();

	if (operating)
	{
		sf::RectangleShape backdrop(sf::Vector2f(bounds.width, bounds.height));
		backdrop.setPosition(bounds.left, bounds.top);
		backdrop.setFillColor(sf::Color::Black);
		Window::GetInstance().Draw(backdrop);

		if (terrainReady)
		{
			DrawLayer(terrainTexture);
		}

		if (shaded)
		{
			DrawLayer(shadeTexture);
		}

		DrawItems();

		if (zoomOut)
		{
			DrawView();
		}

		DrawSweep();
	}
	else
	{
		DrawStatus();
	}

	DrawReveals();
}

void Minimap::DrawLayer(const sf::Texture &texture)
{
	Layer layer;
	layer.texture = &texture;

	const float right = origin.x + (area.width * scale);
	const float bottom = origin.y + (area.height * scale);

	layer.vertices[0] = sf::Vertex(sf::Vector2f(origin.x, origin.y), sf::Vector2f(area.left, area.top));
	layer.vertices[1] = sf::Vertex(sf::Vector2f(right, origin.y), sf::Vector2f(area.left + area.width, area.top));
	layer.vertices[2] = sf::Vertex(sf::Vector2f(right, bottom), sf::Vector2f(area.left + area.width, area.top + area.height));
	layer.vertices[3] = sf::Vertex(sf::Vector2f(origin.x, bottom), sf::Vector2f(area.left, area.top + area.height));

	Window::GetInstance().Draw(layer);
}

void Minimap::Mark(float x, float y, float width, float height, sf::Color colour)
{
	const float left = std::max(x, origin.x);
	const float top = std::max(y, origin.y);
	const float right = std::min(x + width, origin.x + (area.width * scale));
	const float bottom = std::min(y + height, origin.y + (area.height * scale));

	if (right <= left || bottom <= top)
	{
		return;
	}

	marks.append(sf::Vertex(sf::Vector2f(left, top), colour));
	marks.append(sf::Vertex(sf::Vector2f(right, top), colour));
	marks.append(sf::Vertex(sf::Vector2f(right, bottom), colour));
	marks.append(sf::Vertex(sf::Vector2f(left, bottom), colour));
}

void Minimap::DrawItems()
{
	WorldState &world = WorldState::GetInstance();

	const String team = world.GetTeam();
	const bool blind = world.IsBlindView();
	const float dot = std::max(2.0f, scale);

	marks.clear();

	for (const auto &resource : world.resources)
	{
		if (!resource || !Sees(resource->GetX(), resource->GetY(), 1))
		{
			continue;
		}

		const sf::Vector2f at = ToScreen(resource->GetX(), resource->GetY());

		Mark(at.x - (dot / 2.0f), at.y - (dot / 2.0f), dot, dot, RESOURCE);
	}

	for (int pass = 0; pass < 2; pass++)
	{
		for (const auto &item : world.items)
		{
			if (!item || item->GetLife() <= 0.0f || item->GetHidden())
			{
				continue;
			}

			const float centreX = item->GetCenterX();
			const float centreY = item->GetCenterY();
			const float halfWidth = centreX - item->GetX();
			const float halfHeight = centreY - item->GetY();
			const bool footprint = halfWidth > 0.0f && halfHeight > 0.0f;

			if (footprint != (pass == 0))
			{
				continue;
			}

			const bool own = item->GetTeam() == team && !blind;

			if (!own && !Sees(centreX, centreY, 2))
			{
				continue;
			}

			const sf::Color colour = own ? (item->IsSelected() ? SELECTED : OWN) : ENEMY;

			if (footprint)
			{
				const sf::Vector2f at = ToScreen(item->GetX(), item->GetY());

				Mark(at.x, at.y, std::max(2.0f, halfWidth * 2.0f * scale), std::max(2.0f, halfHeight * 2.0f * scale), colour);
			}
			else
			{
				const sf::Vector2f at = ToScreen(centreX, centreY);

				Mark(at.x - (dot / 2.0f), at.y - (dot / 2.0f), dot, dot, colour);
			}
		}
	}

	Window::GetInstance().Draw(marks);
}

void Minimap::DrawView()
{
	const WorldState &world = WorldState::GetInstance();
	const float cell = static_cast<float>(Globals::grid_size);

	const sf::Vector2f topLeft = ToScreen(CameraLeft(world) / cell, CameraTop(world) / cell);
	const sf::Vector2f bottomRight = ToScreen((CameraLeft(world) + ViewWidth(world)) / cell, (CameraTop(world) + ViewHeight(world)) / cell);

	const float left = std::max(topLeft.x, origin.x);
	const float top = std::max(topLeft.y, origin.y);
	const float right = std::min(bottomRight.x, origin.x + (area.width * scale));
	const float bottom = std::min(bottomRight.y, origin.y + (area.height * scale));

	if (right - left < 2.0f || bottom - top < 2.0f)
	{
		return;
	}

	sf::RectangleShape view(sf::Vector2f(right - left, bottom - top));
	view.setPosition(left, top);
	view.setFillColor(sf::Color::Transparent);
	view.setOutlineColor(VIEW);
	view.setOutlineThickness(-1.0f);

	Window::GetInstance().Draw(view);
}

void Minimap::DrawSweep()
{
	if (!sweeping)
	{
		return;
	}

	const float progress = sweepClock.getElapsedTime().asSeconds() / SWEEP_SECONDS;

	if (progress >= 1.0f)
	{
		sweeping = false;
		return;
	}

	Window &window = Window::GetInstance();

	const float line = bounds.top + (progress * bounds.height);

	sf::RectangleShape unswept(sf::Vector2f(bounds.width, bounds.top + bounds.height - line));
	unswept.setPosition(bounds.left, line);
	unswept.setFillColor(sf::Color::Black);
	window.Draw(unswept);

	sf::RectangleShape beam(sf::Vector2f(bounds.width, 2.0f));
	beam.setPosition(bounds.left, line - 1.0f);
	beam.setFillColor(SWEEP);
	window.Draw(beam);
}

void Minimap::DrawReveals()
{
	WorldState &world = WorldState::GetInstance();

	const float pulse = std::fmod(pulseClock.getElapsedTime().asSeconds(), 1.0f);

	for (const RevealedArea &revealed : world.GetRevealedAreas())
	{
		const sf::Vector2f at = ToScreen(static_cast<float>(revealed.x), static_cast<float>(revealed.y));

		if (!OverMap(at.x, at.y))
		{
			continue;
		}

		const float radius = std::clamp(static_cast<float>(revealed.radius) * scale, 3.0f, MAX_PING_RADIUS) * (0.5f + (pulse * 0.5f));

		sf::Color colour = PING;
		colour.a = static_cast<std::uint8_t>(255.0f * (1.0f - (pulse * 0.6f)));

		sf::CircleShape ping(radius);
		ping.setOrigin(radius, radius);
		ping.setPosition(at);
		ping.setFillColor(sf::Color::Transparent);
		ping.setOutlineColor(colour);
		ping.setOutlineThickness(1.0f);

		Window::GetInstance().Draw(ping);
	}
}

void Minimap::DrawStatus()
{
	Window &window = Window::GetInstance();

	sf::RectangleShape blank(sf::Vector2f(bounds.width, bounds.height));
	blank.setPosition(bounds.left, bounds.top);
	blank.setFillColor(sf::Color::Black);
	window.Draw(blank);

	if (!standing)
	{
		return;
	}

	const String text = "NO POWER";
	const unsigned int size = 12;
	const float width = window.MeasureText(text, size);

	window.DrawText(text, {std::floor(bounds.left + ((bounds.width - width) / 2.0f)), std::floor(bounds.top + (bounds.height / 2.0f) - 8.0f)}, size, NO_POWER);
}

bool Minimap::OverMap(float x, float y) const
{
	return x >= origin.x && x < origin.x + (area.width * scale) && y >= origin.y && y < origin.y + (area.height * scale);
}

sf::Vector2f Minimap::ToCell(float x, float y) const
{
	return {
		std::clamp(area.left + ((x - origin.x) / scale), 0.0f, static_cast<float>(gridWidth) - 0.01f),
		std::clamp(area.top + ((y - origin.y) / scale), 0.0f, static_cast<float>(gridHeight) - 0.01f)};
}

sf::Vector2f Minimap::ToScreen(float cellX, float cellY) const
{
	return {origin.x + ((cellX - area.left) * scale), origin.y + ((cellY - area.top) * scale)};
}

bool Minimap::Sees(float cellX, float cellY, std::uint8_t level) const
{
	if (!shaded)
	{
		return true;
	}

	const int x = static_cast<int>(std::floor(cellX));
	const int y = static_cast<int>(std::floor(cellY));

	if (x < 0 || y < 0 || x >= gridWidth || y >= gridHeight)
	{
		return false;
	}

	return WorldState::GetInstance().sightGrid[(static_cast<size_t>(y) * gridWidth) + x] >= level;
}
} // namespace TGX
