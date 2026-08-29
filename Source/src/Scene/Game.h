#pragma once

#include <SFML/Graphics.hpp>
#include "Asset.h"
#include "Background/Background.h"
#include "Economy.h"
#include "FogOfWar.h"
#include "Grid/Grid.h"
#include "Interface.h"
#include "Item.h"
#include "Lookup.h"
#include "Navigation.h"
#include "Projectile.h"
#include "Scene.h"
#include "SkirmishSetup.h"
#include "WayPoints/WayPoints.h"
#include "io/Loader.h"

namespace TGX
{
class Game : public Scene
{
private:
	const static int PANNING_THRESHOLD = 40;
	float PANNING_SPEED = 600;
	sf::Font font;
	sf::Text fpsText;
	sf::Text drawVehiclesCollision;
	unsigned int frame = 0;
	unsigned int currentOrderId = 0;

protected:
	Unique<Loader> loader;
	Unique<Background> background;
	Unique<Item> item;
	Unique<Grid> grid;
	Unique<WayPoints> wayPoints;

	Vector<Unique<Item>> gameItems;
	Vector<Unique<Resource>> gameResources;
	Vector<Unique<Projectile>> gameProjectiles;
	Vector<Unique<Asset>> gameAssets;
	Vector<Unique<Interface>> gameInterfaces;
	Vector<Unique<Triggers>> gameTriggers;
	Vector<Unique<Economy>> gameEconomies;
	Unique<FogOfWar> fogOfWarModule;

	int currentLevel = 0;

public:
	Game();
	~Game() override;
	void Init() override;
	void Update() override;
	void Draw() override;
	void Click() override;
	void RightClick() override;
	void Release() override;
	void Close() override;
	void Free() override;
	void AddGameItem(json &item);
	void RemoveGameItem(json &item);

private:
	void HandlePanning();
	void HandleSingleSelection();
	void ClearSelection();
	std::optional<nlohmann::json> LoadJsonFile(std::string_view filePath);

protected:
	bool LoadExtraResources(int currentLevel, json &level, json &requiredJsons);
};
} // namespace TGX
