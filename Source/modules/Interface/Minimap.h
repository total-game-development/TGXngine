#pragma once

#include <SFML/Graphics.hpp>
#include <cstdint>
#include "Core.h"

namespace TGX
{
class Minimap
{
private:
	sf::FloatRect bounds;
	bool configured = false;

	int gridWidth = 0;
	int gridHeight = 0;

	bool zoomOut = false;
	sf::FloatRect area;
	float scale = 1.0f;
	sf::Vector2f origin;

	sf::Texture terrainTexture;
	bool terrainReady = false;

	sf::Texture shadeTexture;
	Vector<std::uint8_t> shadePixels;
	int shadeRevision = -1;
	bool shaded = false;

	sf::VertexArray marks;

	bool operating = false;
	bool standing = false;
	bool sweeping = false;
	sf::Clock sweepClock;
	sf::Clock pulseClock;

	bool holding = false;
	bool dragging = false;
	sf::Vector2f pressed;

	void Prepare();
	void Frame();
	void Track();
	void DrawLayer(const sf::Texture &texture);
	void Mark(float x, float y, float width, float height, sf::Color colour);
	void DrawItems();
	void DrawView();
	void DrawSweep();
	void DrawReveals();
	void DrawStatus();

	bool OverMap(float x, float y) const;
	sf::Vector2f ToCell(float x, float y) const;
	sf::Vector2f ToScreen(float cellX, float cellY) const;
	bool Sees(float cellX, float cellY, std::uint8_t level) const;

public:
	Minimap();
	void Load(const String &name);
	void Reset();
	void Draw();
	bool Click();
};
} // namespace TGX
