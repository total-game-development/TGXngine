#pragma once

#include <SFML/Graphics.hpp>
#include <nlohmann/json.hpp>
#include "Core.h"
#include "ItemInstance.h"

namespace TGX
{
using FNPTR_AWAKE = ItemInstance *(*)(const String &);
using FNPTR_CREATE = void (*)(Vector<sf::Sprite *> *, Vector<sf::Texture *> *);
using FNPTR_SEND_ORDER = void (*)(ItemInstance *);
using FNPTR_PROCESS_ORDER = void (*)(ItemInstance *);
using FNPTR_RENDER_WINDOW = void (*)(ItemInstance *, Vector<sf::Sprite *> *);
using FNPTR_UPDATE = void (*)(ItemInstance *, Vector<sf::Sprite *> *);
using FNPTR_DELETE = void (*)(int);

using namespace nlohmann;

class Item
{
private:
	FNPTR_AWAKE awake;
	FNPTR_CREATE create;
	FNPTR_SEND_ORDER sendOrders;
	FNPTR_PROCESS_ORDER processOrders;
	FNPTR_RENDER_WINDOW draw;
	FNPTR_UPDATE update;
	FNPTR_DELETE _delete;

	ItemInstance *itemInstance = nullptr;
	int removeUID = 0;

	// A sprite is only put where its item stands by the module's Update, so
	// one drawn before the first Update is at the corner of the screen.
	bool placed = false;

	Vector<Unique<sf::Sprite>> sprites;
	Vector<Unique<sf::Texture>> textures;
	Vector<sf::Sprite *> spritePtrs;

public:
	Item(
		FNPTR_AWAKE awake,
		FNPTR_CREATE create,
		FNPTR_SEND_ORDER sendOrders,
		FNPTR_PROCESS_ORDER processOrders,
		FNPTR_RENDER_WINDOW draw,
		FNPTR_UPDATE update,
		FNPTR_DELETE _delete);

	~Item();

	Item(const Item &) = delete;
	Item &operator=(const Item &) = delete;

	bool Create(json &data);
	void SendOrders();
	void ProcessOrders();
	void Draw();
	void Update();
	void Delete();

	ItemInstance *GetItemInstance();
	void SetRemoteUID(int uid);
	int GetRemoteUID() const;
	void Clear();
};
} // namespace TGX
