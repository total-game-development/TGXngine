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
#include "MultiplayerSetup.h"
#include "Net/Session.h"
#include "ShellModule.h"
#include "UIModule.h"
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
	Vector<Unique<AI>> gameAis;
	Unique<FogOfWar> fogOfWarModule;
	Unique<UIModule> uiModule;
	Unique<ShellModule> shellModule;

	int currentLevel = 0;

	String outcome;
	sf::FloatRect exitButton;

	// Sampled once a second so the overlay can show income, not just balance.
	Map<String, int> lastSampledCash;
	Map<String, int> cashPerSecond;

	// Folded over the commands this client has applied and the ticks it applied
	// them on. It must match what the server folds, byte for byte, or every
	// sanity check reads as a desync -- see Simulation.h in TGXngineServer.
	// One tick of simulated time, matching the rate the server counts at. A
	// networked step advances by exactly this, never by the frame it was drawn
	// in: two machines never render at the same rate, and a simulation paid in
	// real time would move the same unit a different distance on each of them.
	static constexpr float TICK_SECONDS = 1.0f / 60.0f;

	static constexpr std::uint64_t DIGEST_OFFSET = 0xCBF29CE484222325ULL;

	std::uint64_t digest = DIGEST_OFFSET;
	std::int64_t digestTick = 0;

	// The world as this client holds it. Two clients that have stayed in step
	// fold to the same number; any difference in a unit's position, however
	// small, changes it. Compared against the other client's, since the server
	// has no world of its own yet.
	std::uint64_t WorldDigest() const;

	void MixDigest(std::uint64_t value);
	void MixDigestText(const String &text);

public:
	Game();
	~Game() override;
	void Init() override;
	void Update() override;
	void Draw() override;
	void Click() override;
	void RightClick() override;
	bool Text(unsigned int codepoint);
	bool Key(int code);

	// One player's order, resolved onto the world. Single player calls it the
	// moment the order is given; a networked match calls it when the tick the
	// server stamped comes round, so every client resolves it at once.
	void ApplyCommand(const Vector<int> &uids, const json &orders);

	// One tick of simulation. Update runs it freely in single player and only
	// when the lockstep clock allows in a networked match.
	void Step();
	void Release() override;
	void Close() override;
	void Free() override;
	void AddGameItem(json &item);
	void SetOutcome(const String &result);
	void RemoveGameItem(json &item);

private:
	void HandlePanning();
	void DrawOutcome();
	void DrawEconomy();
	void SampleEconomy();
	void HandleSingleSelection();
	void ClearSelection();
	std::optional<nlohmann::json> LoadJsonFile(std::string_view filePath);

protected:
	bool LoadExtraResources(int currentLevel, json &level, json &requiredJsons);
};
} // namespace TGX
