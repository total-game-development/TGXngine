#include <algorithm>
#include <cmath>
#include "AI.h"
#include "AIStates.h"
#include "Core.h"
#include "Enums.h"
#include "Flags.h"
#include "Globals.h"
#include "ItemStates.h"
#include "Logs.h"
#include "Lookup.h"
#include "Orders.h"
#include "StringUtils.hpp"
#include "Utils.hpp"
#include "WorldState.h"

namespace TGX
{
namespace
{
constexpr int plotWidth = 16;
constexpr int plotHeight = 15;
constexpr int plotSearchRings = 40;
constexpr int commandInterval = 60;
constexpr int musterSize = 5;
} // namespace

void BuilderAIState::Awake()
{
}

void BuilderAIState::Start()
{
	Log::Print(StringConcat("BuilderAI commanding ", team, " with $", std::to_string(cash)));
}

void BuilderAIState::Update()
{
	if (team.empty())
	{
		return;
	}

	if (pending)
	{
		buildCounter++;

		if (buildCounter >= pending->buildTime)
		{
			Issue(pending, pendingX, pendingY);

			cash -= pending->cost;
			pending = nullptr;
			buildCounter = 0;
		}
	}
	else
	{
		Ref<BuildNode> next = NextBuild();

		if (next)
		{
			if (next->type == "buildings" || next->type == "turrets")
			{
				if (FindPlot(pendingX, pendingY))
				{
					pending = next;
				}
			}
			else
			{
				pendingX = -1;
				pendingY = -1;
				pending = next;
			}

			buildCounter = 0;
		}
	}

	CommandArmy();
}

int BuilderAIState::Owned(const String &name) const
{
	WorldState &world = WorldState::GetInstance();

	int count = 0;

	for (const auto &item : world.items)
	{
		if (item && item->GetTeam() == team && item->GetName() == name)
		{
			count++;
		}
	}

	return count;
}

int BuilderAIState::OwnedStructures() const
{
	WorldState &world = WorldState::GetInstance();

	int count = 0;

	for (const auto &item : world.items)
	{
		if (item && item->GetTeam() == team && item->GetType() == "buildings")
		{
			count++;
		}
	}

	return count;
}

Ref<BuildNode> BuilderAIState::NextBuild()
{
	if (OwnedStructures() < buildLimit)
	{
		for (const auto &node : orderedNodes)
		{
			if (node->type != "buildings" && node->type != "turrets")
			{
				continue;
			}

			if (Owned(node->name) > 0 || cash < node->cost)
			{
				continue;
			}

			if (node->parent && Owned(node->parent->name) == 0)
			{
				continue;
			}

			return node;
		}
	}

	if (orderedNodes.empty())
	{
		return nullptr;
	}

	for (std::size_t i = 0; i < orderedNodes.size(); i++)
	{
		const std::size_t at = (trainCursor + i) % orderedNodes.size();
		const Ref<BuildNode> &node = orderedNodes[at];

		if (node->type == "buildings" || node->type == "turrets")
		{
			continue;
		}

		if (!node->parent || Owned(node->parent->name) == 0 || cash < node->cost)
		{
			continue;
		}

		trainCursor = (at + 1) % orderedNodes.size();

		return node;
	}

	return nullptr;
}

bool BuilderAIState::IsPlotClear(int x, int y) const
{
	WorldState &world = WorldState::GetInstance();

	if (x < 0 || y < 0 ||
		x + plotWidth > Globals::mapGridWidth ||
		y + plotHeight > Globals::mapGridHeight)
	{
		return false;
	}

	for (int cellY = y; cellY < y + plotHeight; cellY++)
	{
		for (int cellX = x; cellX < x + plotWidth; cellX++)
		{
			if (world.currentTerrainMapPassableGrid[cellY][cellX] >= Flags::CELL_COLLISION_MODE_HARD)
			{
				return false;
			}
		}
	}

	return true;
}

bool BuilderAIState::FindPlot(int &outX, int &outY) const
{
	WorldState &world = WorldState::GetInstance();

	float sumX = 0.0f;
	float sumY = 0.0f;
	int structures = 0;

	for (const auto &item : world.items)
	{
		if (item && item->GetTeam() == team && item->GetType() == "buildings")
		{
			sumX += item->GetX();
			sumY += item->GetY();
			structures++;
		}
	}

	if (structures == 0)
	{
		return false;
	}

	const int centreX = static_cast<int>(sumX / static_cast<float>(structures));
	const int centreY = static_cast<int>(sumY / static_cast<float>(structures));

	for (int ring = 2; ring <= plotSearchRings; ring++)
	{
		for (int offsetY = -ring; offsetY <= ring; offsetY++)
		{
			for (int offsetX = -ring; offsetX <= ring; offsetX++)
			{
				if (std::abs(offsetX) != ring && std::abs(offsetY) != ring)
				{
					continue;
				}

				if (IsPlotClear(centreX + offsetX, centreY + offsetY))
				{
					outX = centreX + offsetX;
					outY = centreY + offsetY;

					return true;
				}
			}
		}
	}

	return false;
}

void BuilderAIState::Issue(const Ref<BuildNode> &node, int x, int y) const
{
	WorldState &world = WorldState::GetInstance();

	String command = StringConcat("command:", "build");
	command += "," + StringConcat("name:", node->name);
	command += "," + StringConcat("type:", node->type);
	command += "," + StringConcat("team:", team);

	if (x >= 0 && y >= 0)
	{
		command += "," + StringConcat("x:", std::to_string(x));
		command += "," + StringConcat("y:", std::to_string(y));
	}

	Log::Print(StringConcat("AI Command Generated: ", command));

	world.gameEvents.emplace_back(UIAction::AddGameItem, command);
}

void BuilderAIState::CommandArmy()
{
	commandCounter++;

	if (commandCounter < commandInterval)
	{
		return;
	}

	commandCounter = 0;

	WorldState &world = WorldState::GetInstance();

	float musterX = 0.0f;
	float musterY = 0.0f;
	int soldiers = 0;

	for (const auto &item : world.items)
	{
		if (item && item->GetTeam() == team && item->CanAttack())
		{
			musterX += item->GetX();
			musterY += item->GetY();
			soldiers++;
		}
	}

	if (soldiers < musterSize)
	{
		return;
	}

	musterX /= static_cast<float>(soldiers);
	musterY /= static_cast<float>(soldiers);

	int targetUid = -1;
	float targetDistance = 0.0f;

	for (const auto &item : world.items)
	{
		if (!item || item->GetTeam() == team || item->GetLife() <= 0.0f || !item->IsAttackable())
		{
			continue;
		}

		const float deltaX = item->GetX() - musterX;
		const float deltaY = item->GetY() - musterY;
		const float distance = (deltaX * deltaX) + (deltaY * deltaY);

		if (targetUid == -1 || distance < targetDistance)
		{
			targetUid = item->GetUid();
			targetDistance = distance;
		}
	}

	if (targetUid == -1)
	{
		return;
	}

	for (const auto &item : world.items)
	{
		if (!item || item->GetTeam() != team || !item->CanAttack())
		{
			continue;
		}

		const Orders::Order order = item->GetOrders()->order;

		if (order != Orders::Order::Stand && order != Orders::Order::Standing)
		{
			continue;
		}

		if (item->GetState() == ItemStates::Attacking && LookUp::Get(item->GetTargetUid()) != -1)
		{
			continue;
		}

		item->SetState(ItemStates::Attacking);
		item->SetTargetUid(targetUid);
		item->SetOrders(Orders::Order::Move);
	}
}

void BuilderAIState::InitialiseMapTechTree(const nlohmann::json &aiOpponentData)
{
	Log::Print("BuilderAIState parsing map tech tree...");

	// 1. Clear out any residual data if re-entering a level
	buildTemplates.clear();
	rootNodes.clear();
	orderedNodes.clear();

	// 2. Extract global configuration limits cleanly
	if (aiOpponentData.contains("buildLimit"))
	{
		buildLimit = aiOpponentData["buildLimit"].get<int>();
	}

	if (!aiOpponentData.contains("techTree") || !aiOpponentData["techTree"].contains("nodes"))
	{
		Log::Warning("Provided AI opponent data lacks a valid techTree structure.");
		return;
	}

	const auto &jsonNodes = aiOpponentData["techTree"]["nodes"];

	// FIRST PASS: Allocate all BuildNodes and populate their raw values
	for (const auto &nodeJson : jsonNodes)
	{
		if (!nodeJson.contains("name"))
		{
			Log::Warning("Skipping invalid tech tree node entry missing a 'name' field.");
			continue;
		}

		auto node = std::make_shared<BuildNode>();
		node->name = nodeJson["name"].get<String>();
		node->type = nodeJson.value("type", "buildings");
		node->role = nodeJson.value("role", "");
		node->cost = nodeJson.value("cost", 0);
		node->powerUsage = nodeJson.value("powerUsage", 0);
		node->buildTime = nodeJson.value("buildTime", 0);
		node->isRoot = nodeJson.value("isRoot", false);

		// Temporarily store children names to establish linkages in our second pass
		if (nodeJson.contains("children") && nodeJson["children"].is_array())
		{
			for (const auto &childName : nodeJson["children"])
			{
				node->childrenNames.push_back(childName.get<String>());
			}
		}

		// Cache the node in our master map registry
		buildTemplates[node->name] = node;

		// Keep track of base starting points for immediate traversal
		if (node->isRoot)
		{
			rootNodes.push_back(node);
		}
	}

	// SECOND PASS: Reconstruct hierarchy by resolving string identifiers to direct references
	int linkedEdgesCount = 0;
	for (auto &[name, parentNode] : buildTemplates)
	{
		for (const String &childName : parentNode->childrenNames)
		{
			// Verify the referenced child node actually exists in our data mapping
			auto it = buildTemplates.find(childName);
			if (it != buildTemplates.end())
			{
				Ref<BuildNode> &childNode = it->second;

				// Establish the explicit parent pointer reference link
				childNode->parent = parentNode;
				linkedEdgesCount++;
			}
			else
			{
				Log::Warning("Tech Tree Layout Mismatch: Node '" + name +
							 "' references missing child node target signature: '" + childName + "'");
			}
		}
	}

	Log::Success("Tech Tree Generation Complete: Successfully compiled " +
				 std::to_string(buildTemplates.size()) + " nodes and mapped " +
				 std::to_string(linkedEdgesCount) + " structural pointer linkages.");

	OrderNodesBreadthFirst();
	DisplayTechTree();
}

void BuilderAIState::OrderNodesBreadthFirst()
{
	orderedNodes.clear();

	Vector<Ref<BuildNode>> frontier = rootNodes;

	while (!frontier.empty())
	{
		Vector<Ref<BuildNode>> nextFrontier;

		for (const auto &node : frontier)
		{
			if (std::ranges::find(orderedNodes, node) != orderedNodes.end())
			{
				continue;
			}

			orderedNodes.push_back(node);

			for (const String &childName : node->childrenNames)
			{
				auto it = buildTemplates.find(childName);

				if (it != buildTemplates.end())
				{
					nextFrontier.push_back(it->second);
				}
			}
		}

		frontier = std::move(nextFrontier);
	}

	for (const auto &[name, node] : buildTemplates)
	{
		if (std::ranges::find(orderedNodes, node) == orderedNodes.end())
		{
			orderedNodes.push_back(node);
		}
	}
}

void BuilderAIState::DisplayTechTree() const
{
	Log::Print("--------------------------------------------------");
	Log::Print("         LIVE AI TECH TREE VISUALIZATION          ");
	Log::Print("--------------------------------------------------");

	if (rootNodes.empty())
	{
		Log::Warning("Cannot display tech tree: No root nodes have been registered.");
		return;
	}

	// Process every independent tree root found in the configuration payload
	for (const auto &root : rootNodes)
	{
		PrintNodeRecursive(root, 0);
	}

	Log::Print("--------------------------------------------------");
}

void BuilderAIState::PrintNodeRecursive(const Ref<BuildNode> &node, int depth) const
{
	if (!node)
	{
		return;
	}

	// 1. Build a dynamic indentation indent visual based on tree depth
	String indentation = "";
	for (int i = 0; i < depth; ++i)
	{
		indentation += (i == depth - 1) ? "|-- " : "|   ";
	}

	// 2. Format details about this node
	String nodeInfo = indentation + node->name + " [" + node->role + "]";
	nodeInfo += " (Cost: " + std::to_string(node->cost) + ", Time: " + std::to_string(node->buildTime) + ")";

	Log::Print(nodeInfo);

	// 3. Traverse using our direct pointer layout map!
	for (const auto &[name, potentialChild] : buildTemplates)
	{
		// FIXED: Direct evaluation check comparing the parent shared_ptr with the current node shared_ptr
		if (potentialChild && potentialChild->parent == node)
		{
			PrintNodeRecursive(potentialChild, depth + 1);
		}
	}
}

} // namespace TGX
