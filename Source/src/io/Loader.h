#pragma once

#include <nlohmann/json.hpp>
#include "AI.h"
#include "Asset.h"
#include "Core.h"
#include "Economy.h"
#include "FogOfWar.h"
#include "Interface.h"
#include "Item.h"
#include "Projectile.h"
#include "Resource.h"
#include "Triggers.h"

#ifdef PLATFORM_WINDOWS
#include <Windows.h>
#define LOAD_LIBRARY(path) LoadLibraryA(path.c_str())
#define GET_PROC(handle, name) GetProcAddress(static_cast<HMODULE>(handle), name)
#define FREE_LIBRARY(handle) FreeLibrary(static_cast<HMODULE>(handle))
#endif

#if defined(PLATFORM_LINUX) || defined(PLATFORM_APPLE)
#include <dlfcn.h>
#define LOAD_LIBRARY(path) dlopen(path.c_str(), RTLD_LAZY)
#define GET_PROC(handle, name) dlsym(handle, name)
#define FREE_LIBRARY(handle) dlclose(handle)
#endif

using namespace nlohmann;

namespace TGX
{
using FNPTR_INIT = void (*)();
using FNPTR_DESTROY = void (*)(const String &);

class Loader
{
private:
	Vector<Unique<Item>> gameItems;
	Vector<Unique<Resource>> gameResources;
	Vector<Unique<Asset>> gameAssets;
	Vector<Unique<Projectile>> gameProjectiles;
	Vector<Unique<Interface>> gameInterfaces;
	Vector<Unique<Triggers>> gameTriggers;
	Vector<Unique<Economy>> gameEconomies;
	Vector<Unique<AI>> gameAis;
	Unique<FogOfWar> fogOfWarModule = nullptr;
#ifdef PLATFORM_WINDOWS
	Map<String, HINSTANCE> dlls;
#endif
#if defined(PLATFORM_LINUX) || defined(PLATFORM_APPLE)
	Map<String, void *> dlls;
#endif
	FNPTR_INIT init;
	FNPTR_DESTROY destroy;
	static inline int uid = 0;

public:
	Loader();
	void AssignGameDLLs(json &requirements);
	void AssignGameItems(json &level);
	void AssignGameResources(json &level);
	void AssignProjectiles(json &level);
	void AssignInterface(json &level);
	void AssignTriggers(json &level);
	void AssignEconomy(json &level);
	void AssignAI(json &level);
	void AssignFogOfWar();
	void AddGameItem(int uid, json &item);
	void ReleaseGameDLLs(json &level, json &requirements);
	void ResetUIDCounter();
	Vector<Unique<Item>> &GetGameItems();
	Vector<Unique<Asset>> &GetGameAssets();
	Vector<Unique<Resource>> &GetGameResources();
	Vector<Unique<Projectile>> &GetGameProjectiles();
	Vector<Unique<Interface>> &GetGameInterfaces();
	Vector<Unique<Economy>> &GetEconomies();
	Vector<Unique<Triggers>> &GetGameTriggers();
	Vector<Unique<AI>> &GetAIs();
	Unique<FogOfWar> &GetFogOfWar();
};
} // namespace TGX
