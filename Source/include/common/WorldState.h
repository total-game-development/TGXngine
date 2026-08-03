#pragma once

#include <utility>
#include "AssetState.h"
#include "Core.h"
#include "EconomyInstance.h"
#include "Enums.h"
#include "ItemInstance.h"
#include "ItemPair.h"
#include "Point.h"
#include "ProjectileInstance.h"
#include "ResourceInstance.h"
#include "common_export.h"

namespace TGX
{
class WorldState
{
private:
	float mouseX = 0;
	float mouseY = 0;
	float gameX = 0;
	float gameY = 0;
	float panX = 0;
	float panY = 0;
	float mapXOffset = 0;
	float mapYOffset = 0;
	float backgroundOffsetX = 0;
	float backgroundOffsetY = 0;
	float backgroundOffsetWidth = 0;
	float backgroundOffsetHeight = 0;
	int numberOfHorizontalTiles = 0;
	int numberOfVerticalTiles = 0;
	int targetFps;
	float fps = 0.0f;
	float deltaTime = 0.0f;
	int currentLevel = 0;
	int currentOrderId = 0;
	int mapGridWidth = 0;
	int mapGridHeight = 0;
	int canvasWidthOffset = 0;
	int canvasHeightOffset = 0;
	int productionWidth = 1920;
	int productionHeight = 1080;
	int canvasWidth = 1040;
	int canvasHeight = 720;
	bool update = false;
	bool leftClick = false;
	bool rightClick = false;
	bool itemUnderCursor = false;
	bool enemyItemUnderCursor = false;
	bool loadableItemUnderCursor = false;
	bool resourceUnderCursor = false;
	bool skipSelectionRemoval = false;
	bool placement = false;
	bool built = false;
	bool triggered = false;
	bool production = true;
	bool debugOnScreen = false;
	bool fogOfWarEnabled = true;
	bool closed = false;
	int itemThatIsUnderCursor = 0;
	int cash = 0;
	int powerUsage = 0;
	int powerTotal = 0;
	SceneType currentScene = SceneType::Intro;
	String command;
	String extendedPath = "/";
	String team;

public:
	Vector<int> selected;
	Vector<Unique<ItemInstance>> items;
	Vector<Unique<ResourceInstance>> resources;
	Vector<Unique<EconomyInstance>> economies;
	Vector<Unique<AssetState>> assets;
	Vector<ItemPair> itemPairs;
	Vector<String> logs;
	Vector<String> warnings;
	Vector<String> errors;
	Vector<Pair<float, float>> activeItemPositions;
	Vector<Pair<UIAction, String>> gameEvents;
	Map<String, Vector<Unique<ProjectileInstance>>> projectiles;
	Map<String, Map<String, int>> extractors;
	Map<int, std::tuple<int, int, int, int>> uids_grid;
	Map<String, int> cells_grid;
	String pendingQueue;
	String commandQueue;
	Vector<Vector<int>> currentMapTerrainGrid;
	Vector<Vector<int>> currentTerrainMapPassableGrid;
	Vector<Vector<int>> currentIsleMapPassableGrid;
	Vector<Vector<int>> currentTerrainMapLookupTable;
	Vector<Vector<int>> wayPoints;
	Set<Point> terrainMarkers;
	Map<String, Unique<ProjectileInstance>> projectileRegistry;
	Map<int, Vector<Tuple<float, float, int>>> deployMap;
	Map<String, Vector<int>> static_cells;
	Map<String, int> primaryItems;
	Map<int, int> lookupMap;

	float GetGameX() const
	{
		return gameX;
	}

	void SetGameX(float inGameX)
	{
		gameX = inGameX;
	}

	float GetGameY() const
	{
		return gameY;
	}

	void SetGameY(float inGameY)
	{
		gameY = inGameY;
	}

	float GetMouseX() const
	{
		return mouseX;
	}

	void SetMouseX(float inMouseX)
	{
		mouseX = inMouseX;
	}

	float GetMouseY() const
	{
		return mouseY;
	}

	void SetMouseY(float inMouseY)
	{
		mouseY = inMouseY;
	}

	float GetPanX() const
	{
		return panX;
	}
	void SetPanX(float inPanX)
	{
		panX = inPanX;
	}
	float GetPanY() const
	{
		return panY;
	}
	void SetPanY(float inPanY)
	{
		panY = inPanY;
	}

	float GetMapXOffset() const
	{
		return mapXOffset;
	}
	void UpdateMapXOffset(float inMapXOffset)
	{
		this->mapXOffset += inMapXOffset;
	}
	float GetMapYOffset() const
	{
		return mapYOffset;
	}
	void UpdateMapYOffset(float inMapYOffset)
	{
		mapYOffset += inMapYOffset;
	}
	void RestMapOffset()
	{
		this->mapXOffset = 0;
		this->mapYOffset = 0;
	}

	int GetProductionWidth() const
	{
		return productionWidth;
	}
	int GetProductionHeight() const
	{
		return productionHeight;
	}

	float GetFPS() const
	{
		return fps;
	}

	void SetFPS(float inFps)
	{
		fps = inFps;
	}

	void SetTargetFPS(int inTargetFps)
	{
		targetFps = inTargetFps;
	}

	int GetTargetFPS()
	{
		return targetFps;
	}

	float GetDeltaTime() const
	{
		return deltaTime;
	}

	void SetDeltaTime(float inDeltaTime)
	{
		deltaTime = inDeltaTime;
	}

	int GetCurrentLevel() const
	{
		return currentLevel;
	}

	void SetCurrentLevel(int inCurrentLevel)
	{
		currentLevel = inCurrentLevel;
	}

	int IncrementOrderId()
	{
		return this->currentOrderId = (this->currentOrderId + 1) % 65536;
	}

	void SetBackgroundOffsetX(float inBackgroundOffsetX)
	{
		backgroundOffsetX = inBackgroundOffsetX;
	}

	float GetBackgroundOffsetX() const
	{
		return backgroundOffsetX;
	}

	void SetBackgroundOffsetY(float inBackgroundOffsetY)
	{
		backgroundOffsetY = inBackgroundOffsetY;
	}

	float GetBackgroundOffsetY() const
	{
		return backgroundOffsetY;
	}

	void SetBackgroundOffsetWidth(float inBackgroundOffsetWidth)
	{
		backgroundOffsetWidth = inBackgroundOffsetWidth;
	}
	float GetBackgroundOffsetWidth() const
	{
		return backgroundOffsetWidth;
	}

	void SetBackgroundOffsetHeight(float inBackgroundOffsetHeight)
	{
		backgroundOffsetHeight = inBackgroundOffsetHeight;
	}

	int GetMapGridWidth() const
	{
		return mapGridWidth;
	}

	void SetMapGridWidth(int inMapGridWidth)
	{
		mapGridWidth = inMapGridWidth;
	}

	int GetMapGridHeight() const
	{
		return mapGridHeight;
	}

	void SetMapGridHeight(int inMapGridHeight)
	{
		mapGridHeight = inMapGridHeight;
	}

	void SetCanvasSize(int inCanvasWidth, int inCanvasHeight)
	{
		canvasWidth = inCanvasWidth;
		canvasHeight = inCanvasHeight;
	}

	int GetCanvasWidth() const
	{
		return canvasWidth;
	}

	int GetCanvasHeight() const
	{
		return canvasHeight;
	}

	void SetCanvasOffsetSize(int inCanvasWidthOffset, int inCanvasHeightOffset)
	{
		canvasWidthOffset = inCanvasWidthOffset;
		canvasHeightOffset = inCanvasHeightOffset;
	}

	int GetCanvasOffsetWidth() const
	{
		return canvasWidthOffset;
	}

	int GetCanvasOffsetHeight() const
	{
		return canvasHeightOffset;
	}

	float GetBackgroundOffsetHeight() const
	{
		return backgroundOffsetHeight;
	}

	void SetNumberOfHorizontalTiles(int inNumberOfHorizontalTiles)
	{
		numberOfHorizontalTiles = inNumberOfHorizontalTiles;
	}

	int GetNumberOfHorizontalTiles() const
	{
		return numberOfHorizontalTiles;
	}

	void SetNumberOfVerticalTiles(int inNumberOfVerticalTiles)
	{
		numberOfVerticalTiles = inNumberOfVerticalTiles;
	}

	int GetNumberOfVerticalTiles() const
	{
		return numberOfVerticalTiles;
	}

	bool SkipSelectionRemoval() const
	{
		return skipSelectionRemoval;
	}

	void SetSkipSelectionRemoval(bool inSkipSelectionRemoval)
	{
		skipSelectionRemoval = inSkipSelectionRemoval;
	}

	bool IsProduction() const
	{
		return production;
	}

	void SetProduction(bool inProduction)
	{
		production = inProduction;
	}

	void SetDebugOnScreen(bool inDebugOnScreen)
	{
		debugOnScreen = inDebugOnScreen;
	}

	bool IsDebugOnScreen() const
	{
		return debugOnScreen;
	}

	void SetFogOfWarEnabled(bool inFogOfWarEnabled)
	{
		fogOfWarEnabled = inFogOfWarEnabled;
	}

	bool IsFogOfWarEnabled() const
	{
		return fogOfWarEnabled;
	}

	bool IsClosed() const
	{
		return closed;
	}

	void SetClosed(bool inClosed)
	{
		closed = inClosed;
	}

	bool IsPlacement() const
	{
		return placement;
	}

	void SetPlacement(bool inPlacement)
	{
		placement = inPlacement;
	}

	bool IsBuilt() const
	{
		return built;
	}

	void SetBuilt(bool inBuilt)
	{
		built = inBuilt;
	}

	bool IsTriggered() const
	{
		return triggered;
	}

	void SetTriggered(bool inTriggered)
	{
		triggered = inTriggered;
	}

	bool IsUpdate() const
	{
		return update;
	}

	void SetUpdate(bool inUpdate)
	{
		update = inUpdate;
	}

	bool IsLeftClicked() const
	{
		return leftClick;
	}

	void SetLeftClicked(bool inLeftClick)
	{
		this->leftClick = inLeftClick;
	}

	bool IsRightClicked() const
	{
		return rightClick;
	}

	void SetRightClicked(bool inRightClick)
	{
		rightClick = inRightClick;
	}

	bool IsItemUnderCursor() const
	{
		return itemUnderCursor;
	}

	void SetItemUnderCursor(bool inItemUnderCursor)
	{
		itemUnderCursor = inItemUnderCursor;
	}

	int GetItemUidThatIsUnderCursor() const
	{
		return itemThatIsUnderCursor;
	}

	void SetItemUidThatIsUnderCursor(int inItemUnderCursor)
	{
		itemThatIsUnderCursor = inItemUnderCursor;
	}

	bool IsEnemyItemUnderCursor() const
	{
		return enemyItemUnderCursor;
	}

	void SetEnemyItemUnderCursor(bool inEnemyItemUnderCursor)
	{
		enemyItemUnderCursor = inEnemyItemUnderCursor;
	}

	bool IsResourceUnderCursor() const
	{
		return resourceUnderCursor;
	}

	void SetResourceUnderCursor(bool inResourceUnderCursor)
	{
		resourceUnderCursor = inResourceUnderCursor;
	}

	int GetResourceUidThatIsUnderCursor() const
	{
		return itemThatIsUnderCursor;
	}

	void SetResourceUidThatIsUnderCursor(int inItemUnderCursor)
	{
		itemThatIsUnderCursor = inItemUnderCursor;
	}

	bool IsLoadableItemUnderCursor() const
	{
		return loadableItemUnderCursor;
	}

	void SetLoadableItemUnderCursor(bool inLoadableItemUnderCursor)
	{
		loadableItemUnderCursor = inLoadableItemUnderCursor;
	}

	int GetCash() const
	{
		return cash;
	}

	void SetCash(int inCash)
	{
		cash = std::max(0, inCash);
	}

	int GetPowerUsage() const
	{
		return powerUsage;
	}

	void SetPowerUsage(int inPowerUsage)
	{
		powerUsage = std::max(0, inPowerUsage);
	}

	int GetPowerTotal() const
	{
		return powerTotal;
	}

	void SetPowerTotal(int inPowerTotal)
	{
		powerTotal = std::max(0, inPowerTotal);
	}

	String GetExtendedPath()
	{
		return extendedPath;
	}

	void SetExtendedPath(const String &inExtendedPath)
	{
		extendedPath = inExtendedPath;
	}

	void SetExtendedPath(String &&inExtendedPath)
	{
		extendedPath = std::move(inExtendedPath);
	}

	SceneType GetCurrentScene()
	{
		return currentScene;
	}
	void SetCurrentScene(SceneType inCurrentScene)
	{
		currentScene = inCurrentScene;
	}

	String GetCurrentCommand()
	{
		return command;
	}
	void SetCurrentCommand(const String &inCommand)
	{
		command = inCommand;
	}

	void SetCurrentCommand(String &&inCommand)
	{
		command = std::move(inCommand);
	}

	String GetTeam()
	{
		return team;
	}

	void SetTeam(const String &inTeam)
	{
		team = inTeam;
	}

	void SetTeam(String &&inTeam)
	{
		team = std::move(inTeam);
	}

	int GetPrimaryItem(const String &name)
	{
		return primaryItems[name];
	}

	void SetPrimaryItems(const String &name, int index)
	{
		this->primaryItems[name] = index;
	}

	Map<int, int> GetLookup() const
	{
		return lookupMap;
	}

	void SetLookup(int uid, int index)
	{
		this->lookupMap[uid] = index;
	}

	void Clear()
	{
		// Reset simple fields
		gameX = 0;
		gameY = 0;
		mouseX = 0;
		mouseY = 0;
		panX = 0;
		panY = 0;
		mapXOffset = 0;
		mapYOffset = 0;
		backgroundOffsetX = 0;
		backgroundOffsetY = 0;
		backgroundOffsetWidth = 0;
		backgroundOffsetHeight = 0;
		mapGridWidth = 0;
		mapGridHeight = 0;
		update = false;
		leftClick = false;
		rightClick = false;
		itemUnderCursor = false;
		enemyItemUnderCursor = false;
		skipSelectionRemoval = false;
		placement = false;
		built = false;
		triggered = false;
		command = "";

		// Clear containers without deleting pointers
		selected.clear();
		items.clear();
		resources.clear();
		economies.clear();
		assets.clear();
		projectiles.clear();
		itemPairs.clear();

		// Clear other structures
		activeItemPositions.clear();
		gameEvents.clear();
		pendingQueue.clear();
		commandQueue.clear();
		projectileRegistry.clear();
		deployMap.clear();

		for (auto &row : currentMapTerrainGrid)
		{
			row.clear();
		}

		currentMapTerrainGrid.clear();

		for (auto &row : currentTerrainMapPassableGrid)
		{
			row.clear();
		}

		currentTerrainMapPassableGrid.clear();

		for (auto &row : currentIsleMapPassableGrid)
		{
			row.clear();
		}

		currentIsleMapPassableGrid.clear();

		for (auto &row : currentTerrainMapLookupTable)
		{
			row.clear();
		}

		currentTerrainMapLookupTable.clear();

		for (auto &row : wayPoints)
		{
			row.clear();
		}

		wayPoints.clear();

		static_cells.clear();
		primaryItems.clear();
		lookupMap.clear();

		logs.clear();
		warnings.clear();
		errors.clear();
	}

	// singleton
	COMMON_API static WorldState &GetInstance();

	WorldState(const WorldState &) = delete;
	WorldState &operator=(const WorldState &) = delete;

	WorldState(WorldState &&) = delete;
	WorldState &operator=(WorldState &&) = delete;

private:
	WorldState() = default;
	~WorldState() = default;
};
} // namespace TGX
