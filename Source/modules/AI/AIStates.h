#pragma once

#include <nlohmann/json.hpp> // Ensure json support is available to the parsing signature
#include <functional>
#include "Core.h"

namespace TGX
{

struct BuildNode
{
	String name;
	String type;
	String role;
	int cost = 0;
	int powerUsage = 0;
	int buildTime = 0;
	bool isRoot = false;
	Vector<String> childrenNames;	 // Dynamic evaluation tracking array from maps.json
	Ref<BuildNode> parent = nullptr; // Explicitly references other build components
};

struct EconomyState
{
	int cash = 0;
	int extractorCount = 0;
	int waterExtractorCount = 0;
	int oilExtractorCount = 0;
	int water = 0;
	int oil = 0;
};

struct AIProfile
{
	String team;
	int powerUsage = 0;
	int powerTotal = 0;
	EconomyState economy;
	Map<String, int> count;
	Map<String, int> max;
	Map<String, Any> primary;
};

struct BuildingRecord
{
	int x = 0;
	int y = 0;
	int w = 0;
	int h = 0;
	String type;
	String team;
	String role;
};

struct PlacementPoint
{
	int x = 0;
	int y = 0;
};

class BaseBuilder;
class BuildPlanner;

class AIState
{
public:
	AIState() = default;
	virtual ~AIState() = default;

	virtual void Awake() = 0;
	virtual void Start() = 0;
	virtual void Update() = 0;

	// Default fallback interface implementation allows any state
	// block to gracefully ignore or intercept config data on initialization loop.
	virtual void InitialiseMapTechTree(const nlohmann::json &aiOpponentData) {}

	void SetTeam(const String &inTeam)
	{
		team = inTeam;
	}

	const String &GetTeam() const
	{
		return team;
	}

	void SetCash(int inCash)
	{
		cash = inCash;
	}

	int GetCash() const
	{
		return cash;
	}

protected:
	String team;
	int cash = 0;
};

class BuilderAIState : public AIState
{
public:
	void Awake() override;
	void Start() override;
	void Update() override;

	// Explicit override to capture and handle tech tree map generation payloads
	void InitialiseMapTechTree(const nlohmann::json &aiOpponentData) override;

	void DisplayTechTree() const;

private:
	// Recursive internal walker helper
	void PrintNodeRecursive(const Ref<BuildNode> &node, int depth) const;

	void OrderNodesBreadthFirst();

	int Owned(const String &name) const;
	int OwnedStructures() const;

	Ref<BuildNode> NextBuild();
	bool IsPlotClear(int x, int y) const;
	bool FindPlot(int &outX, int &outY) const;
	void Issue(const Ref<BuildNode> &node, int x, int y) const;
	void CommandArmy();

	Map<String, Ref<BuildNode>> buildTemplates;
	Vector<Ref<BuildNode>> rootNodes;
	Vector<Ref<BuildNode>> orderedNodes;

	Ref<BuildNode> pending = nullptr;
	int pendingX = -1;
	int pendingY = -1;
	int buildCounter = 0;
	int buildLimit = 6;
	std::size_t trainCursor = 0;
	int commandCounter = 0;
};

class PlexAIState : public AIState
{
public:
	void Awake() override;
	void Start() override;
	void Update() override;
};

// Define our dynamic creator function signature allocation type cleanly
using StateFactory = std::function<AIState *()>;

} // namespace TGX
