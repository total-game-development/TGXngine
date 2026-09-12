#pragma once

#include <SFML/Graphics.hpp>
#include <nlohmann/json.hpp>
#include <cstdint>
#include "Core.h"

using namespace nlohmann;

namespace TGX::UI
{
enum class ElementType : std::uint8_t
{
	Unknown,
	Container,
	Text,
	Button,
	IconButton,
	TextInput,
	Window
};

ElementType ElementTypeFromString(const String &value);

struct Action
{
	String condition;
	String value;

	bool Empty() const
	{
		return condition.empty();
	}
};

struct Element
{
	ElementType type = ElementType::Unknown;

	String name;
	String path;
	String text;
	String title;
	String placeholder;

	String x = "0";
	String y = "0";

	float width = 120.0f;
	float height = 40.0f;
	float scale = 1.0f;
	float anchorX = 0.0f;
	float anchorY = 0.0f;

	unsigned int fontSize = 16;

	sf::Color fill = sf::Color(0x33, 0x66, 0xFF);
	sf::Color textFill = sf::Color::White;
	sf::Color stroke = sf::Color::Transparent;
	float strokeWidth = 0.0f;

	bool positioned = false;
	bool password = false;
	bool close = true;
	bool mobile = true;

	Action action;

	Vector<Element> elements;
};

sf::Color ColourFromJson(const json &source, const String &key, sf::Color fallback);
Element ElementFromJson(const json &source);
Vector<Element> ElementsFromJson(const json &source);
} // namespace TGX::UI
