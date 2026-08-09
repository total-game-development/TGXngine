#include "Item.h"
#include <algorithm> // for std::ranges::sort
#include <utility>
#include "Config.h"
#include "Logs.h"
#include "Lookup.h"
#include "WorldState.h"

namespace TGX
{

Item::Item(
	FNPTR_AWAKE awake,
	FNPTR_CREATE create,
	FNPTR_SEND_ORDER sendOrders,
	FNPTR_PROCESS_ORDER processOrders,
	FNPTR_RENDER_WINDOW draw,
	FNPTR_UPDATE update,
	FNPTR_DELETE _delete)
	: awake(awake), create(create), sendOrders(sendOrders),
	  processOrders(processOrders), draw(draw), update(update), _delete(_delete)
{
	Log::Info("Item Controller Initialized");
}

Item::~Item()
{
	Log::Clean("Cleaning Item Controller");

	sprites.clear();
	textures.clear();
	spritePtrs.clear();

	itemInstance = nullptr;

	Log::Clean("Cleaning Item Controller Post");
}

bool Item::Create(json &data)
{
	if (!awake)
	{
		return false;
	}

	String name = data.value("name", "Unknown");
	int uid = data.value("uid", -1);

	Unique<ItemInstance> instance{awake(name)};

	if (!instance)
	{
		Log::Error("Awake failed for item: " + name);
		return false;
	}

	itemInstance = instance.get();

	itemInstance->SetUid(uid);
	itemInstance->SetTeam(data.value("team", "Neutral"));
	itemInstance->SetType(data.value("type", "Generic"));
	itemInstance->SetName(name);

	LookUp::Add(uid);

	if (data.contains("x") && !data["x"].is_null())
	{
		itemInstance->SetX(data["x"]);
		itemInstance->SetY(data["y"]);
	}
	else
	{
		itemInstance->SetTraining(true);
	}

	Vector<sf::Sprite *> rawSprites;
	Vector<sf::Texture *> rawTextures;
	create(&rawSprites, &rawTextures);

	sprites.reserve(rawSprites.size());
	spritePtrs.reserve(rawSprites.size());
	for (auto *s : rawSprites)
	{
		if (s)
		{
			sprites.emplace_back(Unique<sf::Sprite>(s));
			spritePtrs.emplace_back(s);
		}
	}

	WorldState &world = WorldState::GetInstance();

	if (spritePtrs.empty())
	{
		Log::Error(
			"No images for '" + itemInstance->GetTeam() + "/" + itemInstance->GetType() +
			"/" + name + "' - skipping item " + std::to_string(uid));

		if (_delete)
		{
			_delete(uid);
		}

		LookUp::Remove(uid);

		itemInstance = nullptr;
		return false;
	}

	world.items.emplace_back(std::move(instance));

	Log::Success("Item '" + name + "' Created and Registered");
	return true;
}

void Item::SendOrders(const Unique<ItemInstance> &inItemInstance)
{
	if (sendOrders && inItemInstance)
	{
		sendOrders(inItemInstance.get());
	}
	else
	{
		Log::Warning("SendOrders: Received an empty Unique pointer or null callback.");
	}
}

void Item::ProcessOrders(const Unique<ItemInstance> &inItemInstance)
{
	if (processOrders && inItemInstance)
	{
		processOrders(inItemInstance.get());
	}
}

void Item::Draw()
{
	if (draw && itemInstance && !spritePtrs.empty())
	{
		draw(itemInstance, &spritePtrs);
	}
}

void Item::Update()
{
	if (!update || !itemInstance)
	{
		return;
	}

	update(itemInstance, &spritePtrs);
}

ItemInstance *Item::GetItemInstance()
{
	return itemInstance;
}

void Item::SetRemoteUID(int uid)
{
	this->removeUID = uid;
}

int Item::GetRemoteUID() const
{
	return this->removeUID;
}

void Item::Clear()
{
	if (itemInstance)
	{
		itemInstance->SetSelected(false);
	}
}

void Item::Delete()
{
	if (_delete && itemInstance)
	{
		_delete(itemInstance->GetUid());
	}
}
} // namespace TGX
