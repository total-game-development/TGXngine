#include "Element.h"
#include <charconv>
#include "Logs.h"

namespace TGX::UI
{
namespace
{
String Coordinate(const json &source, const String &key, const String &fallback)
{
	if (!source.contains(key))
	{
		return fallback;
	}

	const json &value = source[key];

	if (value.is_string())
	{
		return value.get<String>();
	}

	if (value.is_number())
	{
		return std::to_string(value.get<float>());
	}

	return fallback;
}

sf::Color ColourFromString(const String &value, sf::Color fallback)
{
	String digits;

	if (value.size() == 7 && value.front() == '#')
	{
		digits = value.substr(1);
	}
	else if (value.size() == 8 && value.compare(0, 2, "0x") == 0)
	{
		digits = value.substr(2);
	}
	else if (value.size() == 6)
	{
		digits = value;
	}
	else
	{
		return fallback;
	}

	std::uint32_t packed = 0;
	const char *begin = digits.data();
	const char *end = begin + digits.size();

	if (std::from_chars(begin, end, packed, 16).ec != std::errc())
	{
		return fallback;
	}

	return sf::Color(
		static_cast<std::uint8_t>((packed >> 16) & 0xFF),
		static_cast<std::uint8_t>((packed >> 8) & 0xFF),
		static_cast<std::uint8_t>(packed & 0xFF));
}

Action ActionFromJson(const json &source)
{
	Action action;

	if (!source.is_object())
	{
		return action;
	}

	action.condition = source.value("condition", String{});
	action.value = source.value("value", String{});

	return action;
}
} // namespace

ElementType ElementTypeFromString(const String &value)
{
	if (value == "container") { return ElementType::Container; }
	if (value == "text") { return ElementType::Text; }
	if (value == "button") { return ElementType::Button; }
	if (value == "icon-button") { return ElementType::IconButton; }
	if (value == "textInput" || value == "text-input") { return ElementType::TextInput; }
	if (value == "window" || value == "email") { return ElementType::Window; }

	return ElementType::Unknown;
}

sf::Color ColourFromJson(const json &source, const String &key, sf::Color fallback)
{
	if (!source.contains(key))
	{
		return fallback;
	}

	const json &value = source[key];

	if (value.is_string())
	{
		return ColourFromString(value.get<String>(), fallback);
	}

	if (value.is_number_unsigned())
	{
		const auto packed = value.get<std::uint32_t>();

		return sf::Color(
			static_cast<std::uint8_t>((packed >> 16) & 0xFF),
			static_cast<std::uint8_t>((packed >> 8) & 0xFF),
			static_cast<std::uint8_t>(packed & 0xFF));
	}

	return fallback;
}

Element ElementFromJson(const json &source)
{
	Element element;

	if (!source.is_object())
	{
		return element;
	}

	element.type = ElementTypeFromString(source.value("type", String{}));

	element.name = source.value("name", String{});
	element.path = source.value("path", String{});
	element.text = source.value("text", String{});
	element.title = source.value("title", String{});
	element.placeholder = source.value("placeholder", String{});

	element.positioned = source.contains("x") || source.contains("y");

	element.x = Coordinate(source, "x", element.x);
	element.y = Coordinate(source, "y", element.y);

	element.width = source.value("width", element.width);
	element.height = source.value("height", element.height);
	element.scale = source.value("scale", element.scale);

	if (source.contains("anchor"))
	{
		const auto anchor = source.value("anchor", 0.0f);

		element.anchorX = anchor;
		element.anchorY = anchor;
	}

	element.anchorX = source.value("anchorX", element.anchorX);
	element.anchorY = source.value("anchorY", element.anchorY);

	element.fontSize = source.value("fontSize", element.fontSize);

	element.fill = ColourFromJson(source, "fill", element.fill);
	element.textFill = ColourFromJson(source, "textFill", element.textFill);
	element.stroke = ColourFromJson(source, "stroke", element.stroke);
	element.strokeWidth = source.value("strokeWidth", element.strokeWidth);

	element.password = source.value("password", element.password);
	element.close = source.value("close", element.close);
	element.mobile = source.value("mobile", element.mobile);

	if (source.contains("action"))
	{
		element.action = ActionFromJson(source["action"]);
	}

	if (source.contains("elements"))
	{
		element.elements = ElementsFromJson(source["elements"]);
	}

	if (element.type == ElementType::Unknown)
	{
		Log::Warning("UI element of unknown type: " + source.value("type", String{"(missing)"}));
	}

	return element;
}

Vector<Element> ElementsFromJson(const json &source)
{
	Vector<Element> elements;

	if (!source.is_array())
	{
		return elements;
	}

	elements.reserve(source.size());

	for (const auto &entry : source)
	{
		elements.push_back(ElementFromJson(entry));
	}

	return elements;
}
} // namespace TGX::UI
