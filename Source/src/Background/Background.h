#pragma once

#include <SFML/Graphics.hpp>
#include <nlohmann/json.hpp>
#include "Core.h"

using namespace nlohmann;

namespace TGX
{
class Background
{
public:
	Background(json &level);
	~Background();
	void AssignTileNames(const String &mapImages);
	void LoadBackgroundTiles();
	void SampleTerrain(const sf::Image &image, int tileIndex, int gridWidth, int gridHeight);
	void SyncPosition();
	void Draw();
	int GetWidth() const;
	int GetHeight() const;

private:
	Vector<String> tileNames;
	Vector<Unique<sf::Sprite>> backgroundSprites;
	Vector<Unique<sf::Texture>> backgroundTextures;
	Vector<sf::Vector2f> tileAnchors;
	int startingTile;
	int backgroundWidth;
	int backgroundHeight;

public:
	const inline static int rotationXLimit = 880;
	const inline static int rotationYLimit = 500;
};
} // namespace TGX
