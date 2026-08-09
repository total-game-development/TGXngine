#include "Resources.h"
#include "Core.h"
#include "ImageLoader.h"
#include "Logs.h"
#include "Lookup.h"
#include "module_interface.h"

namespace TGX
{
extern "C"
{
	MODULE_API void Init()
	{
		Log::Success("Resources Init");
	}

	MODULE_API ResourceInstance *Awake(const String &name)
	{
		Log::Success("Resources Awake with " + name);

		if (name == "water")
		{
			return globalResource = new WaterState();
		}
		if (name == "oil")
		{
			return globalResource = new OilState();
		}

		return nullptr;
	}

	MODULE_API void Create(
		Vector<sf::Sprite *> *spritesRef,
		Vector<sf::Texture *> *texturesRef)
	{
		resources[globalResource->GetUid()] = std::make_unique<Resources>(globalResource);

		String asset = globalResource->GetType() + "/" + globalResource->GetName();

		for (int i = 0; i < globalResource->GetFrames(); i++)
		{
			String filename = "Resources/images/" + asset + "/" + std::to_string(i) + ".png";

			ImageLoader::Load(filename, globalResource, spritesRef, texturesRef);
		}
	}

	MODULE_API void Start()
	{
		Log::Success("Resources Start");
	}

	MODULE_API void Update(ResourceInstance *resourceInstance, std::vector<sf::Sprite *> *spritesRef)
	{
		WorldState &world = WorldState::GetInstance();

		if (HoverFromCorner(resourceInstance))
		{
			if (world.selected.size() > 0)
			{
				for (int uid : world.selected)
				{
					int index = LookUp::Get(uid);

					if (index == -1)
					{
						continue;
					}

					if (world.items[index]->CanExtract())
					{
						world.SetResourceUnderCursor(true);
						world.SetResourceUidThatIsUnderCursor(resourceInstance->GetUid());
						break;
					}
				}
			}
		}

		int frame = resourceInstance->GetFrame();

		if (spritesRef == nullptr || frame < 0 || frame >= static_cast<int>(spritesRef->size()))
		{
			return;
		}

		(*spritesRef)[frame]->setPosition(
			sf::Vector2f(
				((resourceInstance->GetX() * Globals::grid_size) + static_cast<float>(world.GetMapXOffset())),
				((resourceInstance->GetY() * Globals::grid_size) + static_cast<float>(world.GetMapYOffset()))));
	}

	MODULE_API void Draw(ResourcesStates *resourcesState, Vector<sf::Sprite *> *spritesRef)
	{
		resources[resourcesState->GetUid()]->draw(resourcesState, spritesRef);
	}

	MODULE_API void Clear()
	{
		Log::Success("Resources was cleared");
	}

	MODULE_API void Delete(int uid)
	{
	}

	MODULE_API void Destroy()
	{
	}
}
} // namespace TGX
