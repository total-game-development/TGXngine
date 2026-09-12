#pragma once

#include <SFML/System/Vector2.hpp>
#include "Core.h"

namespace TGX::UI
{
class Layout
{
public:
	static float Resolve(const String &expression, sf::Vector2f view, bool horizontal);
	static sf::Vector2f Resolve(const String &x, const String &y, sf::Vector2f view);
	static sf::Vector2f Anchored(sf::Vector2f position, sf::Vector2f size, float anchorX, float anchorY);
};
} // namespace TGX::UI
