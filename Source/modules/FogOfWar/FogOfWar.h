#include <SFML/Graphics.hpp>
#include <Common.hpp>
#include <algorithm>
#include <cstddef>
#include <cstring>
#include "Core.h"
#include "Globals.h"
#include "ItemInstance.h"
#include "StringUtils.hpp"
#include "Window.h"
#include "WorldState.h"

namespace TGX
{

static constexpr int UPDATE_INTERVAL = 10;
static constexpr int TILE_SIZE = 20;
static constexpr int HALF_TILE = TILE_SIZE / 2;

class FogOfWarState
{
	// cell state
	enum FogState : std::uint8_t
	{
		Shroud,	 // never seen, completely obscured
		Fog,	 // previously seen, slightly darkened
		Visible, // currently seen, completely transparent
	};

	/// cell quadrant state
	enum EdgeTile : std::uint8_t
	{
		EdgeN,
		EdgeS,
		EdgeW,
		EdgeE,
		EdgeNW,
		EdgeNE,
		EdgeSW,
		EdgeSE,

		EdgeCount
	};

	// each cell is split into four quadrants, each samples the
	// two neighbouring cells it touches to choose a tile

	// [quadrant][neighbourA/B][dx, dy]: the two cells a quadrant touches
	static constexpr int QUAD_DIRS[4][2][2] = {
		{{0, -1}, {-1, 0}}, // top-left
		{{0, -1}, {1, 0}},	// top-right
		{{0, 1}, {-1, 0}},	// bottom-left
		{{0, 1}, {1, 0}},	// bottom-right
	};

	// [quadrant][case]: tile to use based on neighbors (corner / first edge / second edge)
	static constexpr int QUAD_TILES[4][3] = {
		{EdgeNW, EdgeN, EdgeW}, // top-left     top    left
		{EdgeNE, EdgeN, EdgeE}, // top-right    top    right
		{EdgeSW, EdgeS, EdgeW}, // bottom-left  bottom left
		{EdgeSE, EdgeS, EdgeE}, // bottom-right bottom right
	};

	// [quadrant][y, x]: pixel origin of each quadrant within a cell
	static constexpr int QUAD_ORIGIN[4][2] = {
		{0, 0},
		{0, HALF_TILE},
		{HALF_TILE, 0},
		{HALF_TILE, HALF_TILE},
	};

	int mapWidth = 0;
	int mapHeight = 0;
	Vector<FogState> fogGrid;
	Vector<std::uint8_t> pixelBuffer;
	sf::Texture fogTexture;
	sf::Sprite fogSprite;
	int texWidth = 0;
	int texHeight = 0;
	int updateCounter = 0;
	std::array<sf::Image, EdgeCount> shroudTiles;
	std::array<sf::Image, EdgeCount> fogTiles;
	bool tilesLoaded = false;
	bool fogDirty = true;
	int lastStartX = -1, lastStartY = -1, lastEndX = -1, lastEndY = -1;

public:
	void Init()
	{
		const auto &world = WorldState::GetInstance();

		mapWidth = world.GetMapGridWidth();
		mapHeight = world.GetMapGridHeight();
		if (mapWidth <= 0 || mapHeight <= 0) { return; }

		fogGrid.assign(static_cast<size_t>(mapWidth) * mapHeight, Shroud);

		// load fog edge tiles
		const std::array<const char *, EdgeCount> edgeNames = {"top", "bottom", "left", "right", "top-left", "top-right", "bottom-left", "bottom-right"};
		tilesLoaded = true;
		for (int i = 0; i < EdgeCount; i++)
		{
			if (shroudTiles[i].loadFromFile(StringConcat("Resources/images/fog/shroud-", edgeNames[i], ".png")))
			{
				shroudTiles[i] = ShrinkHalf(shroudTiles[i]);
			}
			else
			{
				tilesLoaded = false;
			}
			if (fogTiles[i].loadFromFile(StringConcat("Resources/images/fog/fog-", edgeNames[i], ".png")))
			{
				fogTiles[i] = ShrinkHalf(fogTiles[i]);
			}
			else
			{
				tilesLoaded = false;
			}
		}
		if (!tilesLoaded)
		{
			Log::Error("FogOfWar: could not load one or more fog tiles, falling back to solid fill");
		}

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

		// cells currently visible on screen with a one cell margin so theres no seam at the edges
		const int startX = std::clamp(static_cast<int>(std::floor(-world.GetMapXOffset() / Globals::grid_size)) - 1, 0, mapWidth - 1);
		const int endX = std::clamp(static_cast<int>(std::floor((visibleWidth - world.GetMapXOffset()) / Globals::grid_size)) + 1, 0, mapWidth - 1);
		const int startY = std::clamp(static_cast<int>(std::floor(-world.GetMapYOffset() / Globals::grid_size)) - 1, 0, mapHeight - 1);
		const int endY = std::clamp(static_cast<int>(std::floor((visibleHeight - world.GetMapYOffset()) / Globals::grid_size)) + 1, 0, mapHeight - 1);

		const int regionWidth = endX - startX + 1;
		const int regionHeight = endY - startY + 1;
		if (regionWidth <= 0 || regionHeight <= 0) { return; }

		const bool regionChanged = (startX != lastStartX || startY != lastStartY || endX != lastEndX || endY != lastEndY);

		const int bufWidth = regionWidth * TILE_SIZE;
		const int bufHeight = regionHeight * TILE_SIZE;

		// only recreate the texture when the on screen region changes size
		const bool sizeChanged = (bufWidth != texWidth || bufHeight != texHeight);
		if (sizeChanged)
		{
			pixelBuffer.assign(static_cast<size_t>(bufWidth) * bufHeight * 4, 0);
			fogTexture.create(static_cast<unsigned int>(bufWidth), static_cast<unsigned int>(bufHeight));
			fogTexture.setSmooth(false);

			texWidth = bufWidth;
			texHeight = bufHeight;
		}

		if (fogDirty || regionChanged || sizeChanged)
		{
			for (int y = 0; y < regionHeight; y++)
			{
				for (int x = 0; x < regionWidth; x++)
				{
					const int cellY = startY + y;
					const int cellX = startX + x;
					const auto state = At(cellX, cellY);

					if (state == Visible)
					{
						if (!tilesLoaded)
						{
							// no tiles to draw, keep the cell transparent
							ClearTile(x, y);
							continue;
						}

						// fog pass first, shroud pass second
						const int painted = TileCell(fogTiles, Fog, cellX, cellY, x, y, false) | TileCell(shroudTiles, Shroud, cellX, cellY, x, y, false);

						// clear quadrants no tile touches to avoid show last frame
						for (int quadrant = 0; quadrant < 4; quadrant++)
						{
							if ((painted & (1 << quadrant)) == 0)
							{
								ClearQuadrant(x, y, quadrant);
							}
						}
					}
					else
					{
						// fogged cells are solid matching the textures
						const std::uint8_t alpha = state == Shroud ? 255 : 51;

						for (int ty = 0; ty < TILE_SIZE; ty++)
						{
							std::uint8_t *row = &pixelBuffer[(static_cast<size_t>((y * TILE_SIZE) + ty) * bufWidth + static_cast<long>(x) * TILE_SIZE) * 4];
							std::memset(row, 0, static_cast<size_t>(TILE_SIZE) * 4);
							for (int tx = 0; tx < TILE_SIZE; tx++)
							{
								row[(tx * 4) + 3] = alpha;
							}
						}

						// where fog and shroud touch
						if (state == Fog && tilesLoaded)
						{
							TileCell(shroudTiles, Shroud, cellX, cellY, x, y, true);
						}
					}
				}
			}

			fogTexture.update(pixelBuffer.data());

			fogDirty = false;
			lastStartX = startX;
			lastStartY = startY;
			lastEndX = endX;
			lastEndY = endY;
		}

		fogSprite.setTexture(fogTexture);
		fogSprite.setTextureRect(sf::IntRect(0, 0, bufWidth, bufHeight));
		fogSprite.setScale(1.0f, 1.0f);
		fogSprite.setPosition(world.GetMapXOffset() + static_cast<float>(startX * TILE_SIZE), world.GetMapYOffset() + static_cast<float>(startY * TILE_SIZE));

		Window::GetInstance().Draw(fogSprite);
	}

private:
	FogState &At(int x, int y)
	{
		return fogGrid[(static_cast<size_t>(y) * mapWidth) + x];
	}

	const FogState &At(int x, int y) const
	{
		return fogGrid[(static_cast<size_t>(y) * mapWidth) + x];
	}

	// get visibility state of a map cell
	FogState StateAt(int gx, int gy)
	{
		// out of bounds is shroud
		if (gx < 0 || gy < 0 || gx >= mapWidth || gy >= mapHeight)
		{
			return Shroud;
		}

		return At(gx, gy);
	};

	// zero out a whole cell so it is fully transparent
	void ClearTile(int x, int y)
	{
		for (int ty = 0; ty < TILE_SIZE; ty++)
		{
			const size_t idx = (static_cast<size_t>((y * TILE_SIZE) + ty) * texWidth) + (static_cast<long>(x) * TILE_SIZE);
			std::memset(&pixelBuffer[idx * 4], 0, static_cast<size_t>(TILE_SIZE) * 4);
		}
	}

	// zero out one quadrant of a cell
	void ClearQuadrant(int x, int y, int quadrant)
	{
		const int qx = QUAD_ORIGIN[quadrant][1];
		const int qy = QUAD_ORIGIN[quadrant][0];

		for (int ty = 0; ty < HALF_TILE; ty++)
		{
			const size_t idx = (static_cast<size_t>((y * TILE_SIZE) + qy + ty) * texWidth) + (static_cast<long>(x) * TILE_SIZE) + qx;
			std::memset(&pixelBuffer[idx * 4], 0, static_cast<size_t>(HALF_TILE) * 4);
		}
	}

	// copy a half tile quadrant into the pixel buffer
	void BlitQuad(const sf::Image &tile, int x, int y, int qx, int qy)
	{
		const std::uint8_t *src = tile.getPixelsPtr();

		for (int ty = 0; ty < HALF_TILE; ty++)
		{
			const size_t idx = ((y * TILE_SIZE + qy + ty) * texWidth) + (x * TILE_SIZE) + qx;
			const std::uint8_t *dest = src + (static_cast<size_t>(ty) * HALF_TILE * 4);
			const size_t size = static_cast<size_t>(HALF_TILE) * 4;

			std::memcpy(&pixelBuffer[idx * 4], dest, size);
		}
	};

	// alpha composite a tile over the existing buffer so its transparent pixels leave whatever is underneath
	void Composite(const sf::Image &tile, int x, int y, int qx, int qy)
	{
		const unsigned int tileWidth = tile.getSize().x;
		const unsigned int tileHeight = tile.getSize().y;
		const std::uint8_t *src = tile.getPixelsPtr();

		for (unsigned int ty = 0; ty < tileHeight; ty++)
		{
			const size_t idx = ((y * TILE_SIZE + qy + ty) * texWidth) + (x * TILE_SIZE) + qx;
			std::uint8_t *destRow = &pixelBuffer[idx * 4];

			for (unsigned int tx = 0; tx < tileWidth; tx++)
			{
				const std::uint8_t sourceAlpha = src[((ty * tileWidth + tx) * 4) + 3];
				if (sourceAlpha == 0)
				{
					continue;
				}

				const int destAlpha = destRow[(tx * 4) + 3];
				const int outAlpha = sourceAlpha + (destAlpha * (255 - sourceAlpha) / 255);
				if (outAlpha == 0)
				{
					continue;
				}

				for (int channel = 0; channel < 3; channel++)
				{
					const int source = src[((ty * tileWidth + tx) * 4) + channel];
					const int dest = destRow[(tx * 4) + channel];

					const std::uint8_t out = (source * sourceAlpha + dest * destAlpha * (255 - sourceAlpha) / 255) / outAlpha;
					destRow[(tx * 4) + channel] = out;
				}

				destRow[(tx * 4) + 3] = outAlpha;
			}
		}
	};

	// lay out the edge tiles for a cell treating `target` cells as blocked neighbours.
	// returns a bitmask of the quadrants it painted into
	int TileCell(const std::array<sf::Image, EdgeCount> &tiles, FogState target, int cellX, int cellY, int x, int y, bool compositeOverFill)
	{
		int painted = 0;

		for (int quadrant = 0; quadrant < 4; quadrant++)
		{
			const bool primary = StateAt(cellX + QUAD_DIRS[quadrant][0][0], cellY + QUAD_DIRS[quadrant][0][1]) == target;
			const bool secondary = StateAt(cellX + QUAD_DIRS[quadrant][1][0], cellY + QUAD_DIRS[quadrant][1][1]) == target;

			if (!primary && !secondary)
			{
				continue;
			}

			painted |= 1 << quadrant;

			const int tileIndex = (primary && secondary) ? QUAD_TILES[quadrant][0] : (primary ? QUAD_TILES[quadrant][1] : QUAD_TILES[quadrant][2]);

			const int originX = QUAD_ORIGIN[quadrant][1];
			const int originY = QUAD_ORIGIN[quadrant][0];

			if (compositeOverFill)
			{
				Composite(tiles[tileIndex], x, y, originX, originY);
			}
			else
			{
				BlitQuad(tiles[tileIndex], x, y, originX, originY);
			}
		}

		return painted;
	};

	static sf::Image ShrinkHalf(const sf::Image &src)
	{
		sf::Image dst;
		dst.create(HALF_TILE, HALF_TILE, sf::Color::Transparent);
		for (int y = 0; y < HALF_TILE; y++)
		{
			for (int x = 0; x < HALF_TILE; x++)
			{
				// ints cast to u8 later to accumulate the 4 values
				int r = 0, g = 0, b = 0, a = 0;
				for (int dy = 0; dy < 2; dy++)
				{
					for (int dx = 0; dx < 2; dx++)
					{
						const sf::Color c = src.getPixel((x * 2) + dx, (y * 2) + dy);
						r += c.r;
						g += c.g;
						b += c.b;
						a += c.a;
					}
				}

				dst.setPixel(
					x, y,
					sf::Color(static_cast<std::uint8_t>(r / 4),
							  static_cast<std::uint8_t>(g / 4),
							  static_cast<std::uint8_t>(b / 4),
							  static_cast<std::uint8_t>(a / 4)));
			}
		}
		return dst;
	}

	void CalculateVisibility()
	{
		auto &world = WorldState::GetInstance();

		// obscure visible cells
		for (auto &cell : fogGrid)
		{
			if (cell == Visible) { cell = Fog; }
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

		fogDirty = true;
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
					At(x, y) = Visible;
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

		return At(x, y) == Visible;
	}

	bool IsExplored(float worldX, float worldY) const
	{
		int x = static_cast<int>(std::floor(worldX));
		int y = static_cast<int>(std::floor(worldY));

		if (x < 0 || x >= mapWidth || y < 0 || y >= mapHeight)
		{
			return false;
		}

		return At(x, y) != Shroud;
	}
};

FogOfWarState &GetState()
{
	static FogOfWarState state;
	return state;
}

} // namespace TGX
