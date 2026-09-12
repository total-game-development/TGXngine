#pragma once

#include <SFML/Graphics.hpp>
#include "Core.h"

namespace TGX::UI
{
class Textures
{
private:
	static inline Map<String, Unique<sf::Texture>> textures;

public:
	static sf::Texture *Load(const String &path);
	static void Clear();
};
} // namespace TGX::UI
