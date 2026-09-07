#pragma once

#include <cstdint>
#include "Core.h"

namespace TGX::Shell
{
enum class TokenStyle : std::uint8_t
{
	Default,
	Keyword,
	Text,
	Comment,
	Number,
	Boolean
};

struct Span
{
	String text;
	TokenStyle style = TokenStyle::Default;
};

Vector<Vector<Span>> HighlightSource(const Vector<String> &lines);
} // namespace TGX::Shell
