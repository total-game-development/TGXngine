#include "Loader.h"

#include <algorithm>
#include "StringUtils.hpp"
#include "WorldState.h"

namespace
{
std::wstring s2ws(const std::string &s);
}

namespace TGX
{

Loader::Loader()
{
	Log::Success("Created Loader");
}

void Loader::AssignGameDLLs(json &requirements)
{
	Log::Info("Assign Game DLLs");

#if defined(PLATFORM_WINDOWS)
	std::string extension = ".dll";
#elif defined(PLATFORM_LINUX)
	std::string extension = ".so";
#elif defined(PLATFORM_APPLE)
	std::string extension = ".dylib";
#endif

	if (!requirements.contains("required_libraries"))
	{
		Log::Error("JSON requirements does not contain 'required_libraries'");
		return;
	}

	for (auto &required_dll : requirements["required_libraries"])
	{
		String type = required_dll["type"];
		String name = required_dll["name"];

		String filename = std::string(name) + extension;

		Log::Print("Assign Game Type: " + type);
		Log::Print("Assign Game Name: " + name);
		Log::Print("Loader Path: " + filename);

		auto handle = LOAD_LIBRARY(filename);

		if (handle == nullptr)
		{
			Log::Error("Failed to load library: " + filename);
			continue;
		}

		dlls[type] = handle;

		init = (FNPTR_INIT)GET_PROC(handle, "Init");

		if (!init)
		{
			Log::Error("Failed to locate Init function in: " + filename);
		}
		else
		{
			init();
		}
	}
}

void Loader::AssignGameItems(json &level)
{
	Log::Info("Assigning Game Items");

	if (!level.contains("items") || !level["items"].is_array())
	{
		Log::Error("Level JSON missing 'items' array.");
		return;
	}

	json &items = level["items"];
	if (items.empty())
	{
		return;
	}

	gameItems.reserve(items.size());

	for (auto &item : items)
	{
		String type = item.value("type", "Unknown");
		String name = item.value("name", "Unnamed");

		Log::Print("Game Item Type: " + type);
		Log::Print("Game Item Name: " + name);

		if (!dlls.contains(type))
		{
			Log::Error("Missing DLL for item type: " + type);
			continue;
		}

		auto *dllHandle = dlls[type];
		if (!dllHandle)
		{
			Log::Error("Invalid DLL handle for item type: " + type);
			continue;
		}

		auto fnAwake = reinterpret_cast<FNPTR_AWAKE>(GET_PROC(dllHandle, "Awake"));
		auto fnCreate = reinterpret_cast<FNPTR_CREATE>(GET_PROC(dllHandle, "Create"));
		auto fnSendOrder = reinterpret_cast<FNPTR_SEND_ORDER>(GET_PROC(dllHandle, "SendOrders"));
		auto fnProcessOrder = reinterpret_cast<FNPTR_PROCESS_ORDER>(GET_PROC(dllHandle, "ProcessOrders"));
		auto fnDraw = reinterpret_cast<FNPTR_RENDER_WINDOW>(GET_PROC(dllHandle, "Draw"));
		auto fnUpdate = reinterpret_cast<FNPTR_UPDATE>(GET_PROC(dllHandle, "Update"));
		auto fnDelete = reinterpret_cast<FNPTR_DELETE>(GET_PROC(dllHandle, "Delete"));

		if (!fnCreate || !fnDraw || !fnUpdate)
		{
			Log::Error("Missing required function pointers for item type: " + type);
			continue;
		}

		auto newItem = std::make_unique<Item>(
			fnAwake,
			fnCreate,
			fnSendOrder,
			fnProcessOrder,
			fnDraw,
			fnUpdate,
			fnDelete);

		if (!newItem->Create(item))
		{
			Log::Warning(StringConcat("Create() aborted for item: ", name, " (", type, ")"));
			continue;
		}

		if (item.contains("uid"))
		{
			if (ItemInstance *instance = newItem->GetItemInstance())
			{
				instance->SetRemoteUid(item["uid"]);
			}
			else
			{
				Log::Warning("No ItemInstance found when setting UID for item: " + name);
			}
		}

		gameItems.push_back(std::move(newItem));
	}

	auto byPriority = [](const auto &a, const auto &b) {
		return a->GetItemInstance()->GetPriority() < b->GetItemInstance()->GetPriority();
	};

	std::ranges::sort(gameItems, byPriority);

	WorldState &world = WorldState::GetInstance();
	std::ranges::sort(world.items, [](const auto &a, const auto &b) {
		return a->GetPriority() < b->GetPriority();
	});

	for (size_t i = 0; i < gameItems.size(); i++)
	{
		int logicUid = world.items[i]->GetUid();
		int wrapperUid = gameItems[i]->GetItemInstance()->GetUid();
		String name = gameItems[i]->GetItemInstance()->GetName();

		std::string match = (logicUid == wrapperUid) ? "[OK]" : "[MISMATCH]";

		Log::Print(StringConcat(match, " Index ", std::to_string(i),
								" | Logic UID: ", std::to_string(logicUid),
								" | Wrapper UID: ", std::to_string(wrapperUid),
								" | Logic Name: ", name));
	}
}

void Loader::AssignGameResources(json &level)
{
	Log::Info("Assigning Game Resources");

	if (!level.contains("resources") || !level["resources"].is_array())
	{
		Log::Error("Level JSON missing 'resources' array.");
		return;
	}

	json &resources = level["resources"];
	if (resources.empty())
	{
		return;
	}

	gameResources.reserve(resources.size());

	for (auto &resource : resources)
	{
		String type = resource.value("type", "Unknown");
		String name = resource.value("name", "Unnamed");

		Log::Print("Game Resource Type: " + type);
		Log::Print("Game Resource Name: " + name);

		if (!dlls.contains(type))
		{
			Log::Error("Missing DLL for resource type: " + type);
			continue;
		}

		auto *dllHandle = dlls[type];
		if (!dllHandle)
		{
			Log::Error("Invalid DLL handle for resource type: " + type);
			continue;
		}

		auto fnAwake = reinterpret_cast<FNPTR_RESOURCE_AWAKE>(GET_PROC(dllHandle, "Awake"));
		auto fnCreate = reinterpret_cast<FNPTR_RESOURCE_CREATE>(GET_PROC(dllHandle, "Create"));
		auto fnSendOrder = reinterpret_cast<FNPTR_RESOURCE_SEND_ORDER>(GET_PROC(dllHandle, "SendOrders"));
		auto fnProcessOrder = reinterpret_cast<FNPTR_RESOURCE_PROCESS_ORDER>(GET_PROC(dllHandle, "ProcessOrders"));
		auto fnDraw = reinterpret_cast<FNPTR_RESOURCE_RENDER_WINDOW>(GET_PROC(dllHandle, "Draw"));
		auto fnUpdate = reinterpret_cast<FNPTR_RESOURCE_UPDATE>(GET_PROC(dllHandle, "Update"));
		auto fnDelete = reinterpret_cast<FNPTR_RESOURCE_DELETE>(GET_PROC(dllHandle, "Delete"));

		if (!fnCreate || !fnDraw || !fnUpdate)
		{
			Log::Error("Missing required function pointers for resource type: " + type);

			if (!fnCreate)
			{
				Log::Error("Missing required function pointers for resource type: " + type + ", for function - fnCreate");
			}

			if (!fnDraw)
			{
				Log::Error("Missing required function pointers for resource type: " + type + ", for function - fnDraw");
			}

			if (!fnUpdate)
			{
				Log::Error("Missing required function pointers for resource type: " + type + ", for function - fnUpdate");
			}

			continue;
		}

		auto newResource = std::make_unique<Resource>(
			fnAwake,
			fnCreate,
			fnSendOrder,
			fnProcessOrder,
			fnDraw,
			fnUpdate,
			fnDelete);

		Log::Info("newResource created");

		if (!newResource)
		{
			Log::Error("newResource not created");
		}

		if (!newResource->Create(resource))
		{
			Log::Warning(StringConcat("Create() aborted for item: ", name, " (", type, ")"));
			continue;
		}

		gameResources.push_back(std::move(newResource));
	}

	Log::Success("Assigning Game Resources");
}

void Loader::AssignProjectiles(json &level)
{
	Log::Info("Assign Projectiles");

	if (!level.contains("projectiles") || !level["projectiles"].is_array())
	{
		Log::Error("Level JSON missing 'projectiles' array.");
		return;
	}

	json &projectiles = level["projectiles"];
	if (projectiles.empty())
	{
		Log::Warning("Projectiles was missing from this map.");
		return;
	}

	gameProjectiles.reserve(projectiles.size());

	for (auto &item : projectiles)
	{
		String type = item.value("type", "Unknown");
		String name = item.value("name", "Unnamed");

		Log::Print("Game Asset Type: " + type);
		Log::Print("Game Asset Name: " + name);

		if (!dlls.contains(type))
		{
			Log::Error("DLL missing for asset type: " + type);
			continue;
		}

		auto *dllHandle = dlls[type];
		if (!dllHandle)
		{
			Log::Error("Invalid DLL handle for asset type: " + type);
			continue;
		}

		auto awakeFn = reinterpret_cast<FNPTR_PROJECTILE_AWAKE>(GET_PROC(dllHandle, "Awake"));
		auto createFn = reinterpret_cast<FNPTR_PROJECTILE_CREATE>(GET_PROC(dllHandle, "Create"));
		auto updateFn = reinterpret_cast<FNPTR_PROJECTILE_UPDATE>(GET_PROC(dllHandle, "Update"));
		auto drawFn = reinterpret_cast<FNPTR_PROJECTILE_RENDER_WINDOW>(GET_PROC(dllHandle, "Draw"));
		auto clearFn = reinterpret_cast<FNPTR_PROJECTILE_CLEAR>(GET_PROC(dllHandle, "Clear"));

		if (!awakeFn || !createFn || !updateFn || !drawFn || !clearFn)
		{
			Log::Error("Missing function(s) in DLL for type: " + type);
			continue;
		}

		auto newProjectile = std::make_unique<Projectile>(
			awakeFn, createFn, updateFn, drawFn, clearFn);

		if (!newProjectile->Create(item))
		{
			Log::Warning(StringConcat("Create() aborted for asset: ", name, " (", type, ")"));
			continue;
		}

		gameProjectiles.push_back(std::move(newProjectile));
	}
}

void Loader::AssignInterface(json &level)
{
	Log::Info("Assign Interface");

	json &interfaces = level["interface"];

	if (interfaces.size() == 0)
	{
		return;
	}

	json &jsonInterfaceZero = interfaces[0];

	String type = jsonInterfaceZero["type"];
	String name = jsonInterfaceZero["name"];

	Log::Print("Game Interface Type: " + type);
	Log::Print("Game Interface Name: " + name);

	gameInterfaces.push_back(
		std::make_unique<Interface>(
			(FNPTR_INTERFACE_AWAKE)GET_PROC(dlls[jsonInterfaceZero["type"]], "Awake"),
			(FNPTR_INTERFACE_CREATE)GET_PROC(dlls[jsonInterfaceZero["type"]], "Create"),
			(FNPTR_INTERFACE_UPDATE)GET_PROC(dlls[jsonInterfaceZero["type"]], "Update"),
			(FNPTR_INTERFACE_CLICK)GET_PROC(dlls[jsonInterfaceZero["type"]], "Click"),
			(FNPTR_INTERFACE_CLEAR)GET_PROC(dlls[jsonInterfaceZero["type"]], "Clear"),
			(FNPTR_INTERFACE_RENDER_WINDOW)GET_PROC(dlls[jsonInterfaceZero["type"]], "Draw"),
			(FNPTR_INTERFACE_DELETE)GET_PROC(dlls[jsonInterfaceZero["type"]], "Delete")));

	for (auto &jsonInterfaces : interfaces)
	{
		gameInterfaces[0]->Awake(jsonInterfaces["name"]);
		gameInterfaces[0]->Create();
	}
}

void Loader::AssignTriggers(json &level)
{
	Log::Info("Assign Triggers");
	json &triggers = level["triggers"];
	if (triggers.size() == 0)
	{
		return;
	}

	Map<String, Vector<json>> triggersByType;
	for (auto &item : triggers)
	{
		String type = item["type"];
		triggersByType[type].push_back(item);
	}

	for (auto &[type, triggerList] : triggersByType)
	{
		gameTriggers.push_back(
			std::make_unique<Triggers>(
				(FNPTR_TRIGGERS_AWAKE)GET_PROC(dlls[type], "Awake"),
				(FNPTR_TRIGGERS_START)GET_PROC(dlls[type], "Start"),
				(FNPTR_TRIGGERS_CLEAR)GET_PROC(dlls[type], "Clear"),
				(FNPTR_TRIGGERS_DELETE)GET_PROC(dlls[type], "Delete")));

		for (auto &triggerItem : triggerList)
		{
			String name = triggerItem["name"];
			gameTriggers.back()->Awake(name);
		}
	}

	for (auto &gameTrigger : gameTriggers)
	{
		gameTrigger->Start();
	}
}

void Loader::AssignEconomy(json &level)
{
	Log::Info("Assign Economy");

	if (!level.contains("economy"))
	{
		return;
	}

	json &economies = level["economy"];
	if (economies.empty())
	{
		return;
	}

	if (!economies.is_object())
	{
		Log::Error("Loader::AssignEconomy failed - 'economy' must be a JSON object, not an array!");
		return;
	}

	for (auto it = economies.begin(); it != economies.end(); ++it)
	{
		std::string teamName = it.key();
		json &economyData = it.value();

		auto &dll = dlls["economy"];

		auto economy = std::make_unique<TGX::Economy>(
			(FNPTR_ECONOMY_AWAKE)GET_PROC(dll, "Awake"),
			(FNPTR_ECONOMY_START)GET_PROC(dll, "Start"),
			(FNPTR_ECONOMY_UPDATE)GET_PROC(dll, "Update"),
			(FNPTR_ECONOMY_DRAW)GET_PROC(dll, "Draw"),
			(FNPTR_ECONOMY_CLEAR)GET_PROC(dll, "Clear"),
			(FNPTR_ECONOMY_DELETE)GET_PROC(dll, "Delete"));

		int startingCash = economyData.value("cash", 0);
		economy->Awake(teamName, startingCash);

		Log::Info("Economy assigned for " + teamName + " with " + std::to_string(startingCash));

		gameEconomies.push_back(std::move(economy));
	}
}

void Loader::AssignAI(json &level)
{
	Log::Info("Assign AI");

	json &ai = level["ai"];

	if (ai.empty() || !ai.is_array())
	{
		return;
	}

	for (std::size_t i = 0; i < ai.size(); i++)
	{
		json &jsonAIItem = ai[i];

		std::string moduleType = jsonAIItem.contains("type") ? jsonAIItem["type"].get<std::string>() : "ai";

		gameAis.push_back(
			std::make_unique<AI>(
				(FNPTR_AI_AWAKE)GET_PROC(dlls[moduleType], "Awake"),
				(FNPTR_AI_START)GET_PROC(dlls[moduleType], "Start"),
				(FNPTR_AI_UPDATE)GET_PROC(dlls[moduleType], "Update"),
				(FNPTR_AI_CLEAR)GET_PROC(dlls[moduleType], "Clear"),
				(FNPTR_AI_DELETE)GET_PROC(dlls[moduleType], "Delete")));

		gameAis.back()->Awake(jsonAIItem["name"]);
	}

	for (auto &activeAi : gameAis)
	{
		if (activeAi)
		{
			activeAi->Start();
		}
	}
}

void Loader::AddGameItem(int inUid, json &item)
{
	Log::Info("Add Game Item");
	Log::Print("gameItems.size(): " + std::to_string(gameItems.size()));
	Log::Print("game item's uid: " + std::to_string(inUid));

	String type = item["type"];
	Log::Print("game item's type: " + type);

	auto newItem = std::make_unique<Item>(
		(FNPTR_AWAKE)GET_PROC(dlls[item["type"]], "Awake"),
		(FNPTR_CREATE)GET_PROC(dlls[item["type"]], "Create"),
		(FNPTR_SEND_ORDER)GET_PROC(dlls[item["type"]], "SendOrders"),
		(FNPTR_PROCESS_ORDER)GET_PROC(dlls[item["type"]], "ProcessOrders"),
		(FNPTR_RENDER_WINDOW)GET_PROC(dlls[item["type"]], "Draw"),
		(FNPTR_UPDATE)GET_PROC(dlls[item["type"]], "Update"),
		(FNPTR_DELETE)GET_PROC(dlls[item["type"]], "Delete"));

	if (!newItem)
	{
		Log::Error("Failed to allocate new Item");
		return;
	}

	if (!newItem->Create(item))
	{
		Log::Error("Item creation failed, unique_ptr will automatically delete it.");
		return;
	}

	gameItems.push_back(std::move(newItem));
	Log::Success("Game item added successfully.");
}

void Loader::ReleaseGameDLLs(json &level, json &requirements)
{
	json &requiredDlls = requirements["required_libraries"];
	Log::Info("ReleaseGameDLLs: " + std::to_string(requiredDlls.size()));

	json &requiredJsons = level["required_jsons"];

	for (std::size_t i = 0; i < requiredJsons.size(); i++)
	{
		destroy = (FNPTR_DESTROY)GET_PROC(dlls[requiredJsons["type"]], "Destroy");

		destroy(requiredJsons["name"]);
	}

	for (auto &requiredDll : requiredDlls)
	{
		String type = requiredDll["type"];

		bool hasLibraryReleased = FREE_LIBRARY(dlls[type]);

		Log::Clean(type + " Library Released: " + std::to_string(hasLibraryReleased));
	}
}

void Loader::AssignFogOfWar()
{
	Log::Info("Assign FogOfWar");

	if (!dlls.contains("fogofwar"))
	{
		Log::Error("Missing DLL for fogofwar");
		return;
	}

	auto *dllHandle = dlls["fogofwar"];

	auto fnUpdate = (FNPTR_FOGOFWAR_UPDATE)GET_PROC(dllHandle, "Update");
	auto fnDraw = (FNPTR_FOGOFWAR_DRAW)GET_PROC(dllHandle, "Draw");

	fogOfWarModule = std::make_unique<FogOfWar>(fnUpdate, fnDraw);

	Log::Success("FogOfWar assigned");
}

void Loader::ResetUIDCounter()
{
	uid = 0;
}

Vector<Unique<Item>> &Loader::GetGameItems()
{
	return gameItems;
}

Vector<Unique<Asset>> &Loader::GetGameAssets()
{
	return gameAssets;
}

Vector<Unique<Projectile>> &Loader::GetGameProjectiles()
{
	return gameProjectiles;
}

Vector<Unique<Resource>> &Loader::GetGameResources()
{
	return gameResources;
}

Vector<Unique<Interface>> &Loader::GetGameInterfaces()
{
	return gameInterfaces;
}

Vector<Unique<Triggers>> &Loader::GetGameTriggers()
{
	return gameTriggers;
}

Unique<FogOfWar> &Loader::GetFogOfWar()
{
	return fogOfWarModule;
}

Vector<Unique<Economy>> &Loader::GetEconomies()
{
	return gameEconomies;
}

namespace
{
std::wstring s2ws(const std::string &str)
{
	std::wstring result;
	result.reserve(str.size());
	wchar_t wc = 0;
	int bytes = 0;

	for (unsigned char c : str)
	{
		if (c <= 0x7F)
		{
			result.push_back(c);
		}
		else if ((c & 0xE0) == 0xC0)
		{
			wc = c & 0x1F;
			bytes = 1;
		}
		else if ((c & 0xF0) == 0xE0)
		{
			wc = c & 0x0F;
			bytes = 2;
		}
		else if ((c & 0xF8) == 0xF0)
		{
			wc = c & 0x07;
			bytes = 3;
		}
		else if ((c & 0xC0) == 0x80)
		{
			wc = (wc << 6) | (c & 0x3F);
			if (--bytes == 0)
			{
				result.push_back(wc);
			}
		}
	}
	return result;
}
} // namespace

} // namespace TGX
