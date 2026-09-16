#pragma once

#include <SFML/Graphics.hpp>
#include <nlohmann/json.hpp>
#include "SidebarButton.h"

using namespace nlohmann;

namespace TGX
{
class Sidebar
{
private:
	sf::Sprite sidebar;
	sf::Texture sidebarTexture;
	Vector<SidebarButton> buttons;

	void Settle();

public:
	Sidebar();
	~Sidebar();
	void Load(const String &name);
	void Draw();
	void Update();
	void Click();
	void Reset();
	void Restore();
	void Clear();
};
} // namespace TGX
