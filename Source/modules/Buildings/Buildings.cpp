#include "Buildings.h"
#include "BuildingStates.h"
#include "Flags.h"
#include "Globals.h"
#include "ImageLoader.h"
#include "Orders.h"
#include "StringUtils.hpp"
#include "module_interface.h"

namespace TGX
{
extern "C"
{
	MODULE_API void OutputTest()
	{
		Log::Print("This is the buildings output test");
	}

	MODULE_API void Init()
	{
		Log::Info("Buildings init function");
	}

	MODULE_API ItemInstance *Awake(const String &name)
	{
		Log::Info("Awake " + name + " Building");

		if (name == "construction_facility")
		{
			return globalItem = new ConstructionFacilityState();
		}
		if (name == "powerplant")
		{
			return globalItem = new PowerplantState();
		}
		if (name == "science_post")
		{
			return globalItem = new SciencePostState();
		}
		if (name == "barracks")
		{
			return globalItem = new BarracksState();
		}
		if (name == "vehicle_assembly_tunnel")
		{
			return globalItem = new VehicleAssemblyTunnelState();
		}
		if (name == "shipyard")
		{
			return globalItem = new ShipyardState();
		}
		if (name == "airport")
		{
			return globalItem = new AirportState();
		}
		if (name == "oil_extractor")
		{
			return globalItem = new OilExtractor();
		}
		if (name == "water_extractor")
		{
			return globalItem = new WaterExtractor();
		}

		return nullptr;
	}

	MODULE_API void Create(
		std::vector<sf::Sprite *> *spritesRef,
		std::vector<sf::Texture *> *texturesRef)
	{
		Log::Info("Creating the Building's texture and sprite");

		String asset = globalItem->GetTeam() + "/" + globalItem->GetType() + "/" + globalItem->GetName();
		Log::Info("Asset: " + asset);

		Log::Debug("itemInstance->GetFrames(): " + std::to_string(globalItem->GetFrames()));

		for (int i = 0; i < globalItem->GetFrames(); i++)
		{
			String filename = "Resources/images/" + asset + "/" + std::to_string(i) + ".png";
			ImageLoader::Load(filename, globalItem, spritesRef, texturesRef);
		}

		buildings[globalItem->GetUid()] = new Buildings(globalItem);

		auto *buildingState = static_cast<BuildingState *>(globalItem);
		auto &grid = buildingState->passableGrid;
		size_t height = grid.size();
		size_t width = height > 0 ? grid[0].size() : 0;

		float baseX = globalItem->GetX();
		float baseY = globalItem->GetY();

		const auto &deploys = buildingState->GetDeployPositions();

		WorldState &world = WorldState::GetInstance();
		if (!deploys.empty())
		{
			world.deployMap.insert(
				{globalItem->GetUid(), deploys});
		}

		if (world.IsPlacement())
		{
			Log::Info("Building was created with placement");

			auto x = globalItem->GetX();
			auto y = globalItem->GetY();

			globalItem->SetX(x);
			globalItem->SetY(y);

			baseX = globalItem->GetX();
			baseY = globalItem->GetY();

			int placementFrame = globalItem->GetFrame();

			if (spritesRef != nullptr && placementFrame >= 0 && placementFrame < static_cast<int>(spritesRef->size()))
			{
				(*spritesRef)[placementFrame]->setPosition(
					sf::Vector2f(
						x * static_cast<float>(Globals::grid_size + world.GetMapXOffset()),
						y * static_cast<float>(Globals::grid_size + world.GetMapYOffset())));
			}

			world.SetPlacement(false);
		}
		else
		{
			Log::Info("Building was created without placement");
		}

		for (size_t y = 0; y < height; ++y)
		{
			for (size_t x = 0; x < width; ++x)
			{
				int gridX = static_cast<int>(baseX + static_cast<float>(x));
				int gridY = static_cast<int>(baseY + static_cast<float>(y));

				if (gridY < 0 || gridY >= world.GetMapGridHeight() || gridX < 0 || gridX >= world.GetMapGridWidth())
				{
					continue;
				}

				world.currentTerrainMapPassableGrid[gridY][gridX] = Flags::CELL_COLLISION_MODE_HARD;
			}
		}

		globalItem->SetBuildable(true);
		world.SetPrimaryItems(globalItem->GetName(), globalItem->GetUid());

		if (buildingState->GetPowerUsage() < 0)
		{
			world.SetPowerTotal(world.GetPowerTotal() - buildingState->GetPowerUsage());
		}
		else
		{
			world.SetPowerUsage(world.GetPowerUsage() + buildingState->GetPowerUsage());
		}
	}

	MODULE_API void SendOrders(Orders *orders, ItemInstance * /*itemInstance*/)
	{
		Log::Print(StringConcat("This building is processing it's orders ", orders));
	}

	MODULE_API void ProcessOrders()
	{
	}

	MODULE_API void Draw(ItemInstance *itemInstance, std::vector<sf::Sprite *> *spritesRef)
	{
		buildings[itemInstance->GetUid()]->Draw(itemInstance, spritesRef);
	}

	MODULE_API void Update(ItemInstance *itemInstance, std::vector<sf::Sprite *> *spritesRef)
	{
		WorldState &world = WorldState::GetInstance();

		if (HoverFromCorner(itemInstance))
		{
			world.SetItemUnderCursor(true);
			world.SetItemUidThatIsUnderCursor(itemInstance->GetUid());

			if (world.IsLeftClicked())
			{
				Log::Print("Building is clicked");
				itemInstance->SetSelected(true);
				world.selected.emplace_back(itemInstance->GetUid());
			}
		}

		int frame = itemInstance->GetFrame();

		if (spritesRef == nullptr || frame < 0 || frame >= static_cast<int>(spritesRef->size()))
		{
			return;
		}

		(*spritesRef)[frame]->setPosition(
			sf::Vector2f(
				((itemInstance->GetX() * Globals::grid_size) + static_cast<float>(world.GetMapXOffset())),
				((itemInstance->GetY() * Globals::grid_size) + static_cast<float>(world.GetMapYOffset()))));

		buildings[itemInstance->GetUid()]->Update(itemInstance, spritesRef);
	}

	MODULE_API void Delete(int uid)
	{
		auto it = buildings.find(uid);
		if (it != buildings.end() && it->second != nullptr)
		{
			delete it->second;
			buildings.erase(it);
		}

		Log::Print(StringConcat("Current size of the buildings map: ", buildings.size()));
	}

	MODULE_API void Destroy(const String &name)
	{
		Log::Print(StringConcat("Destroy static building { ", name, " } "));
	}
}
} // namespace TGX
