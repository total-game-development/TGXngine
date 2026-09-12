#include "Layout.h"
#include <charconv>
#include "Logs.h"

namespace TGX::UI
{
namespace
{
bool ToFloat(const String &text, float &out)
{
	if (text.empty())
	{
		return false;
	}

	try
	{
		std::size_t consumed = 0;
		const float value = std::stof(text, &consumed);

		if (consumed != text.size())
		{
			return false;
		}

		out = value;
	}
	catch (const std::exception &)
	{
		return false;
	}

	return true;
}

String Trim(const String &text)
{
	const std::size_t begin = text.find_first_not_of(" \t");

	if (begin == String::npos)
	{
		return {};
	}

	const std::size_t end = text.find_last_not_of(" \t");

	return text.substr(begin, end - begin + 1);
}

bool Base(const String &keyword, sf::Vector2f view, bool horizontal, float &out)
{
	if (keyword == "centre" || keyword == "center")
	{
		out = horizontal ? view.x * 0.5f : view.y * 0.5f;
		return true;
	}

	if (keyword == "width" || keyword == "right")
	{
		out = view.x;
		return true;
	}

	if (keyword == "height" || keyword == "bottom")
	{
		out = view.y;
		return true;
	}

	if (keyword == "left" || keyword == "top")
	{
		out = 0.0f;
		return true;
	}

	return ToFloat(keyword, out);
}
} // namespace

float Layout::Resolve(const String &expression, sf::Vector2f view, bool horizontal)
{
	const String trimmed = Trim(expression);

	if (trimmed.empty())
	{
		return 0.0f;
	}

	float direct = 0.0f;

	if (ToFloat(trimmed, direct))
	{
		return direct;
	}

	const std::size_t split = trimmed.find_first_of("+-", 1);

	if (split == String::npos)
	{
		float base = 0.0f;

		if (!Base(trimmed, view, horizontal, base))
		{
			Log::Warning("UI layout expression not understood: " + expression);
		}

		return base;
	}

	float base = 0.0f;

	if (!Base(Trim(trimmed.substr(0, split)), view, horizontal, base))
	{
		Log::Warning("UI layout expression not understood: " + expression);
		return 0.0f;
	}

	float offset = 0.0f;

	if (!ToFloat(Trim(trimmed.substr(split + 1)), offset))
	{
		Log::Warning("UI layout offset not understood: " + expression);
		return base;
	}

	return trimmed[split] == '-' ? base - offset : base + offset;
}

sf::Vector2f Layout::Resolve(const String &x, const String &y, sf::Vector2f view)
{
	return {Resolve(x, view, true), Resolve(y, view, false)};
}

sf::Vector2f Layout::Anchored(sf::Vector2f position, sf::Vector2f size, float anchorX, float anchorY)
{
	return {position.x - (size.x * anchorX), position.y - (size.y * anchorY)};
}
} // namespace TGX::UI
