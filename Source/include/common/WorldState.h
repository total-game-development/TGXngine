#pragma once

#include <random>
#include <utility>
#include "AIDebug.h"
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
// One thing a side is making: paid for when the order landed, advanced a tick
// at a time by every client, and folded into the digest. A unit deploys itself
// when it is done; a building or turret waits, ready, for its owner to say
// where it goes.
struct ProductionOrder
{
	String team;
	String key;
	String type;
	int ticks = 0;
	int progress = 0;
	int stall = 0;
	bool ready = false;
};

// The ground a building kind occupies once it is down: its footprint, and the
// room its deploy berths need below it.
struct BuildSpan
{
	int width = 0;
	int height = 0;
};

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
	bool borderless = false;
	bool headless = false;
	bool debugOnScreen = false;
	bool fogOfWarEnabled = true;
	bool closed = false;
	int itemThatIsUnderCursor = 0;
	int cash = 0;
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

	// What the AI commanders decided this tick, as commands: the uids each is
	// for, and its orders as JSON. A commander never changes the world itself.
	// Single player applies these at once; a networked match's host sends them
	// to be stamped, so every client applies them on the same tick.
	Vector<Pair<Vector<int>, String>> aiCommands;

	// Orders a commander has sent that have not been applied yet. Until they
	// are, the world does not show them, and a commander that could not see its
	// own order would give it again.
	Map<String, int> aiInFlight;

	// What every side is making, in the order it was ordered. Shared state: it
	// changes only when a command lands or a tick is run, the same on every
	// client.
	Vector<ProductionOrder> productionOrders;

	ProductionOrder *FindProduction(const String &inTeam, const String &inKey)
	{
		for (ProductionOrder &order : productionOrders)
		{
			if (order.team == inTeam && order.key == inKey)
			{
				return &order;
			}
		}

		return nullptr;
	}

	const ProductionOrder *FirstProduction(const String &inTeam) const
	{
		for (const ProductionOrder &order : productionOrders)
		{
			if (order.team == inTeam)
			{
				return &order;
			}
		}

		return nullptr;
	}

	// Purchases that have been paid for, waiting to be picked up by whatever
	// raised them. A purchase leaves the machine that clicked and comes back
	// stamped for a tick, so the button cannot start its own timer: it waits
	// here for the tick that took the money.
	Vector<String> settledPurchases;

	// One stream of chance for the whole match, living where every module can
	// reach it. A generator held static inside a header is a separate stream
	// per module, seeded from the machine, which under lockstep sends the same
	// unit somewhere different on each client.
	std::mt19937 random{0x9E3779B9U};

	std::mt19937 &Random()
	{
		return random;
	}

	void SeedRandom(std::uint32_t seed)
	{
		random.seed(seed);
	}

	// Two engine values, so a draw covers any range an int can express without
	// the span arithmetic overflowing.
	std::uint64_t Draw64()
	{
		const std::uint64_t high = static_cast<std::uint32_t>(random());
		const std::uint64_t low = static_cast<std::uint32_t>(random());

		return (high << 32) | low;
	}

	// Reduced here rather than through std::uniform_int_distribution, whose
	// mapping from engine output onto a range is unspecified and differs between
	// standard libraries. mt19937 itself is specified, so a draw taken this way
	// is the same number on every machine in the match. Inclusive of both ends.
	int RandomInt(int minimum, int maximum)
	{
		if (maximum <= minimum)
		{
			return minimum;
		}

		const std::int64_t low = minimum;
		const std::uint64_t span = static_cast<std::uint64_t>(static_cast<std::int64_t>(maximum) - low) + 1;

		const std::uint64_t bucket = (~std::uint64_t{0}) / span;
		const std::uint64_t ceiling = bucket * span;

		std::uint64_t draw = Draw64();

		// The tail that would land unevenly is thrown away rather than folded in,
		// so every value in the range is equally likely on every machine.
		while (draw >= ceiling)
		{
			draw = Draw64();
		}

		return static_cast<int>(low + static_cast<std::int64_t>(draw / bucket));
	}

	// The same, for a fraction. Twenty-four bits is what a float holds exactly,
	// so the division is lossless and lands on the same value everywhere.
	float RandomFloat(float minimum, float maximum)
	{
		const std::uint32_t bits = static_cast<std::uint32_t>(random()) >> 8;
		const float unit = static_cast<float>(bits) / 16777216.0f;

		return minimum + (unit * (maximum - minimum));
	}
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

	// How much ground a building kind needs: its own footprint, and the berths
	// it deploys into. Published by the Buildings module from the states
	// themselves, so the sidebar and the AI ask the building rather than each
	// keeping a figure of their own. Kind data, not world data: a level ending
	// does not clear it.
	Map<String, BuildSpan> buildSpans;
	Map<String, Vector<int>> static_cells;
	Map<String, int> primaryItems;

	// A side's power is its own. Keyed like extractors and primaryItems: a
	// powerplant lights the buildings of whoever raised it and nobody else's.
	Map<String, int> powerUsage;
	Map<String, int> powerTotal;

	// Sides whose grid a hack has cut. Simulation state like the treasuries: a
	// command puts a side in here on a stamped tick, and it stays until the
	// owner restores it or a supplier comes back onto the grid.
	Set<String> powerCuts;
	Map<int, int> lookupMap;

	// Published by the AI module, read by the debug overlay.
	Map<String, AIDebugSnapshot> aiDebug;

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

	bool IsBorderless() const
	{
		return borderless;
	}

	bool IsHeadless() const
	{
		return headless;
	}

	void SetHeadless(bool inHeadless)
	{
		headless = inHeadless;
	}

	void SetProduction(bool inProduction)
	{
		production = inProduction;
	}

	void SetBorderless(bool inBorderless)
	{
		borderless = inBorderless;
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

	EconomyInstance *FindEconomy(const String &inTeam)
	{
		for (const auto &economy : economies)
		{
			if (economy && economy->GetTeam() == inTeam)
			{
				return economy.get();
			}
		}

		return nullptr;
	}

	int GetTeamCash(const String &inTeam)
	{
		const EconomyInstance *treasury = FindEconomy(inTeam);

		return treasury != nullptr ? treasury->GetCash() : 0;
	}

	// The team's purse, not the number on the local player's HUD. Under lockstep
	// every client runs this for every team at the same tick, so the treasury is
	// a value a digest can hold two clients to.
	bool SpendTeamCash(const String &inTeam, int amount)
	{
		EconomyInstance *treasury = FindEconomy(inTeam);

		if (treasury == nullptr || treasury->GetCash() < amount)
		{
			return false;
		}

		treasury->SetCash(treasury->GetCash() - amount);

		if (inTeam == team)
		{
			SetCash(treasury->GetCash());
		}

		return true;
	}

	int GetPowerUsage(const String &inTeam) const
	{
		auto it = powerUsage.find(inTeam);

		return (it == powerUsage.end()) ? 0 : it->second;
	}

	void SetPowerUsage(const String &inTeam, int inPowerUsage)
	{
		powerUsage[inTeam] = std::max(0, inPowerUsage);
	}

	int GetPowerTotal(const String &inTeam) const
	{
		auto it = powerTotal.find(inTeam);

		return (it == powerTotal.end()) ? 0 : it->second;
	}

	void SetPowerTotal(const String &inTeam, int inPowerTotal)
	{
		powerTotal[inTeam] = std::max(0, inPowerTotal);
	}

	bool IsPowerCut(const String &inTeam) const
	{
		return powerCuts.find(inTeam) != powerCuts.end();
	}

	void SetPowerCut(const String &inTeam, bool inCut)
	{
		if (inCut)
		{
			powerCuts.insert(inTeam);
		}
		else
		{
			powerCuts.erase(inTeam);
		}
	}

	// What everything that needs power asks. A side is lit when its grid can
	// carry what is on it and nobody has cut it.
	bool HasPower(const String &inTeam) const
	{
		return !IsPowerCut(inTeam) && GetPowerTotal(inTeam) >= GetPowerUsage(inTeam);
	}

	void ConnectPower(const String &inTeam, int inPowerUsage)
	{
		if (inPowerUsage < 0)
		{
			// A supplier coming onto the grid is a grid being rebuilt, which is
			// the other way back from a cut.
			SetPowerCut(inTeam, false);

			SetPowerTotal(inTeam, GetPowerTotal(inTeam) - inPowerUsage);
		}
		else
		{
			SetPowerUsage(inTeam, GetPowerUsage(inTeam) + inPowerUsage);
		}
	}

	void DisconnectPower(const String &inTeam, int inPowerUsage)
	{
		if (inPowerUsage < 0)
		{
			SetPowerTotal(inTeam, GetPowerTotal(inTeam) + inPowerUsage);
		}
		else
		{
			SetPowerUsage(inTeam, GetPowerUsage(inTeam) - inPowerUsage);
		}
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

	int GetPrimaryItem(const String &inTeam, const String &name)
	{
		auto it = primaryItems.find(inTeam + "/" + name);

		return (it == primaryItems.end()) ? 0 : it->second;
	}

	void SetPrimaryItems(const String &inTeam, const String &name, int uid)
	{
		this->primaryItems[inTeam + "/" + name] = uid;
	}

	Map<int, int> &GetLookup()
	{
		return lookupMap;
	}

	const Map<int, int> &GetLookup() const
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
		aiCommands.clear();
		aiInFlight.clear();
		productionOrders.clear();
		settledPurchases.clear();
		pendingQueue.clear();
		commandQueue.clear();
		projectileRegistry.clear();
		deployMap.clear();
		aiDebug.clear();
		powerUsage.clear();
		powerTotal.clear();
		powerCuts.clear();

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
