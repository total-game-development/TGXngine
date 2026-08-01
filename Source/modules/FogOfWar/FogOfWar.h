#include <SFML/Graphics.hpp>
#include <Common.hpp>
#include <algorithm>
#include <cstddef>
#include "Core.h"
#include "Globals.h"
#include "ItemInstance.h"
#include "StringUtils.hpp"
#include "Window.h"
#include "WorldState.h"

namespace TGX
{

constexpr int UPDATE_INTERVAL = 10;

class FogOfWarState
{
	enum FogState : std::uint8_t
	{
		Shroud,	 // never seen, completely obscured
		Fog,	 // previously seen, slightly darkened
		Visible, // currently seen, completely transparent
	};

	int mapWidth = 0;
	int mapHeight = 0;
	Vector<Vector<FogState>> fogGrid;
	Vector<std::uint8_t> pixelBuffer;
	sf::Texture fogTexture;
	sf::Sprite fogSprite;
	int texWidth = 0;
	int texHeight = 0;
	int updateCounter = 0;

public:
	void Init()
	{
		const auto &world = WorldState::GetInstance();

		mapWidth = world.GetMapGridWidth();
		mapHeight = world.GetMapGridHeight();
		if (mapWidth <= 0 || mapHeight <= 0) { return; }

		fogGrid.assign(mapHeight, Vector<FogState>(mapWidth, Shroud));

		Log::Info(StringConcat("FogOfWar module initialized: ", mapWidth, "x", mapHeight));
	}

	void Update()
	{
		if (mapWidth <= 0 || mapHeight <= 0) { return; }

		auto &world = WorldState::GetInstance();

		// update visible state of enemy items
		for (auto &item : world.items)
		{
			if (item->GetTeam() == world.GetTeam())
			{
				item->setVisible(true);
			}
			else
			{
				item->setVisible(IsVisible(item->GetCenterX(), item->GetCenterY()));
			}
		}

		// visibility update is quite large so only do it every N times
		updateCounter++;
		if (updateCounter < UPDATE_INTERVAL) { return; }
		updateCounter = 0;

		CalculateVisibility();
	}

	void Draw()
	{
		if (mapWidth <= 0 || mapHeight <= 0) { return; }

		WorldState &world = WorldState::GetInstance();

		// actual visible window size
		const sf::Vector2f viewSize = Window::GetInstance().GetViewSize();
		const float visibleWidth = viewSize.x;
		const float visibleHeight = viewSize.y;

		// cells currently visible on screen (with a one cell margin so theres no seam at the edges)
		const int startX = std::clamp(static_cast<int>(std::floor(-world.GetMapXOffset() / Globals::grid_size)) - 1, 0, mapWidth - 1);
		const int endX = std::clamp(static_cast<int>(std::floor((visibleWidth - world.GetMapXOffset()) / Globals::grid_size)) + 1, 0, mapWidth - 1);
		const int startY = std::clamp(static_cast<int>(std::floor(-world.GetMapYOffset() / Globals::grid_size)) - 1, 0, mapHeight - 1);
		const int endY = std::clamp(static_cast<int>(std::floor((visibleHeight - world.GetMapYOffset()) / Globals::grid_size)) + 1, 0, mapHeight - 1);

		const int regionWidth = endX - startX + 1;
		const int regionHeight = endY - startY + 1;
		if (regionWidth <= 0 || regionHeight <= 0) { return; }

		// only recreate the texture when the on-screen region changes size
		if (regionWidth != texWidth || regionHeight != texHeight)
		{
			pixelBuffer.assign(static_cast<size_t>(regionWidth) * regionHeight * 4, 0);
			fogTexture.create(static_cast<unsigned int>(regionWidth), static_cast<unsigned int>(regionHeight));
			fogTexture.setSmooth(false);

			texWidth = regionWidth;
			texHeight = regionHeight;
		}

		// fill pixel data for visible cells only
		for (int y = 0; y < regionHeight; y++)
		{
			for (int x = 0; x < regionWidth; x++)
			{
				// update pixel alpha based on state
				const size_t idx = (static_cast<size_t>(y) * regionWidth + x) * 4;
				const auto state = fogGrid[startY + y][startX + x];

				if (state == Shroud) { pixelBuffer[idx + 3] = 220; }
				else if (state == Fog) { pixelBuffer[idx + 3] = 140; }
				else if (state == Visible) { pixelBuffer[idx + 3] = 0; }
			}
		}

		fogTexture.update(pixelBuffer.data());

		fogSprite.setTexture(fogTexture);
		fogSprite.setTextureRect(sf::IntRect(0, 0, regionWidth, regionHeight));
		fogSprite.setScale(static_cast<float>(Globals::grid_size), static_cast<float>(Globals::grid_size));
		fogSprite.setPosition(world.GetMapXOffset() + static_cast<float>(startX * Globals::grid_size), world.GetMapYOffset() + static_cast<float>(startY * Globals::grid_size));

		Window::GetInstance().Draw(fogSprite);
	}

private:
	void CalculateVisibility()
	{
		auto &world = WorldState::GetInstance();

		// obscure visible cells
		for (auto &vec : fogGrid)
		{
			for (auto &cell : vec)
			{
				if (cell == Visible) { cell = Fog; }
			}
		}

		// mark visible cells around items
		for (size_t i = 0; i < world.items.size(); i++)
		{
			const ItemInstance *item = world.items[i].get();
			if (!item || item->GetTeam() != world.GetTeam() || item->GetHidden()) { continue; }

			const int cx = static_cast<int>(std::round(item->GetCenterX()));
			const int cy = static_cast<int>(std::round(item->GetCenterY()));
			const int sight = item->GetSight();

			MarkVisible(cx, cy, sight);
		}
	}

	void MarkVisible(int cx, int cy, int sight)
	{
		const int sightSq = sight * sight;
		const int x1 = std::max(0, cx - sight);
		const int x2 = std::min(mapWidth - 1, cx + sight);
		const int y1 = std::max(0, cy - sight);
		const int y2 = std::min(mapHeight - 1, cy + sight);

		// circle around point
		for (int y = y1; y <= y2; y++)
		{
			const int dy = y - cy;
			const int dySq = dy * dy;

			for (int x = x1; x <= x2; x++)
			{
				int dx = x - cx;
				if (((dx * dx) + dySq) <= sightSq)
				{
					fogGrid[y][x] = Visible;
				}
			}
		}
	}

	bool IsVisible(float worldX, float worldY) const
	{
		int x = static_cast<int>(std::floor(worldX));
		int y = static_cast<int>(std::floor(worldY));

		if (x < 0 || x >= mapWidth || y < 0 || y >= mapHeight)
		{
			return false;
		}

		return fogGrid[y][x] == Visible;
	}

	bool IsExplored(float worldX, float worldY) const
	{
		int x = static_cast<int>(std::floor(worldX));
		int y = static_cast<int>(std::floor(worldY));

		if (x < 0 || x >= mapWidth || y < 0 || y >= mapHeight)
		{
			return false;
		}

		return fogGrid[y][x] != Shroud;
	}
};

FogOfWarState &GetState()
{
	static FogOfWarState state;
	return state;
}

} // namespace TGX
