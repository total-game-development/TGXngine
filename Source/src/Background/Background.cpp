#include "Background.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include "Globals.h"
#include "Window.h"
#include "WorldState.h"

namespace TGX
{
Background::Background(json &level)
{
	WorldState &world = WorldState::GetInstance();

	startingTile = 0;
	Globals::mapGridWidth = level["mapGridWidth"];
	Globals::mapGridHeight = level["mapGridHeight"];

	backgroundWidth = static_cast<int>(level["mapGridWidth"]) * Globals::grid_size;
	backgroundHeight = static_cast<int>(level["mapGridHeight"]) * Globals::grid_size;

	world.UpdateMapXOffset(level["backgroundOffsetX"]);
	world.UpdateMapYOffset(level["backgroundOffsetY"]);

	world.SetNumberOfHorizontalTiles(level["numberOfHorizontalTiles"]);
	world.SetNumberOfVerticalTiles(level["numberOfVerticalTiles"]);

	AssignTileNames(String(level["mapImage"]));
	LoadBackgroundTiles();
}

Background::~Background() = default;

void Background::LoadBackgroundTiles()
{
	backgroundTextures.reserve(tileNames.size());
	backgroundSprites.reserve(tileNames.size());

	const int gridWidth = backgroundWidth / Globals::grid_size;
	const int gridHeight = backgroundHeight / Globals::grid_size;

	Vector<std::uint8_t> &terrain = WorldState::GetInstance().terrainPixels;
	terrain.assign(static_cast<size_t>(gridWidth) * gridHeight * 4, 0);

	for (size_t alpha = 3; alpha < terrain.size(); alpha += 4)
	{
		terrain[alpha] = 255;
	}

	for (size_t tileIndex = 0; tileIndex < tileNames.size(); tileIndex++)
	{
		const String &tileName = tileNames[tileIndex];

		sf::Image image;
		if (!image.loadFromFile(tileName))
		{
			Log::Warning("Cannot load image: " + tileName);
			continue;
		}

		SampleTerrain(image, static_cast<int>(tileIndex), gridWidth, gridHeight);

		auto texture = std::make_unique<sf::Texture>();
		texture->loadFromImage(image);
		texture->setSmooth(false);
		backgroundTextures.push_back(std::move(texture));

		auto sprite = std::make_unique<sf::Sprite>();
		sprite->setTexture(*backgroundTextures.back());

		sprite->setTextureRect(sf::IntRect(0, 0, rotationXLimit + 1, rotationYLimit + 1));

		backgroundSprites.push_back(std::move(sprite));
	}

	WorldState &world = WorldState::GetInstance();
	int totalTiles = world.GetNumberOfHorizontalTiles() * world.GetNumberOfVerticalTiles();

	tileAnchors.clear();
	tileAnchors.reserve(totalTiles);

	for (int l = 0; l < world.GetNumberOfHorizontalTiles(); l++)
	{
		for (int k = 0; k < world.GetNumberOfVerticalTiles(); k++)
		{
			float x = static_cast<float>(k * rotationXLimit);
			float y = static_cast<float>((l * rotationYLimit) + 80);
			tileAnchors.push_back(sf::Vector2f(x, y));
		}
	}

	SyncPosition();
}

void Background::SampleTerrain(const sf::Image &image, int tileIndex, int gridWidth, int gridHeight)
{
	const int columns = std::max(1, WorldState::GetInstance().GetNumberOfVerticalTiles());
	const int originX = (tileIndex % columns) * rotationXLimit;
	const int originY = (tileIndex / columns) * rotationYLimit;

	const int imageWidth = static_cast<int>(image.getSize().x);
	const int imageHeight = static_cast<int>(image.getSize().y);

	if (imageWidth <= 0 || imageHeight <= 0)
	{
		return;
	}

	Vector<std::uint8_t> &terrain = WorldState::GetInstance().terrainPixels;

	const int size = Globals::grid_size;
	const int firstX = originX / size;
	const int firstY = originY / size;
	const int lastX = std::min(gridWidth, (originX + std::min(imageWidth, rotationXLimit)) / size);
	const int lastY = std::min(gridHeight, (originY + std::min(imageHeight, rotationYLimit)) / size);

	for (int cellY = firstY; cellY < lastY; cellY++)
	{
		for (int cellX = firstX; cellX < lastX; cellX++)
		{
			int red = 0;
			int green = 0;
			int blue = 0;

			for (int sample = 0; sample < 4; sample++)
			{
				const int localX = std::clamp((cellX * size) - originX + ((sample % 2) == 0 ? size / 4 : (3 * size) / 4), 0, imageWidth - 1);
				const int localY = std::clamp((cellY * size) - originY + ((sample / 2) == 0 ? size / 4 : (3 * size) / 4), 0, imageHeight - 1);

				const sf::Color colour = image.getPixel(static_cast<unsigned int>(localX), static_cast<unsigned int>(localY));

				red += colour.r;
				green += colour.g;
				blue += colour.b;
			}

			const size_t at = ((static_cast<size_t>(cellY) * gridWidth) + cellX) * 4;

			terrain[at] = static_cast<std::uint8_t>(red / 4);
			terrain[at + 1] = static_cast<std::uint8_t>(green / 4);
			terrain[at + 2] = static_cast<std::uint8_t>(blue / 4);
		}
	}
}

void Background::SyncPosition()
{
	for (size_t i = 0; i < backgroundSprites.size(); i++)
	{
		float finalX = tileAnchors[i].x - Globals::mapOffsetX;
		float finalY = tileAnchors[i].y - Globals::mapOffsetY;

		backgroundSprites[i]->setPosition(std::round(finalX), std::round(finalY));
	}
}

void Background::AssignTileNames(const String &mapImages)
{
	WorldState &world = WorldState::GetInstance();
	int totalTiles = world.GetNumberOfVerticalTiles() * world.GetNumberOfHorizontalTiles();
	tileNames.reserve(totalTiles);

	for (int i = 0; i < totalTiles; i++)
	{
		tileNames.emplace_back(mapImages + std::to_string(i) + ".png");
	}
}

void Background::Draw()
{
	Window &window = Window::GetInstance();
	for (auto &sprite : backgroundSprites)
	{
		window.Draw(*sprite);
	}
}

int Background::GetWidth() const
{
	return backgroundWidth;
}

int Background::GetHeight() const
{
	return backgroundHeight;
}
} // namespace TGX
