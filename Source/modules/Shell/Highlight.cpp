#include "Highlight.h"
#include "Lexer.h"

namespace TGX::Shell
{
namespace
{
bool IsAlpha(char character)
{
	return (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z');
}

bool IsInt(char character)
{
	return character >= '0' && character <= '9';
}

bool IsBool(const String &identifier)
{
	return identifier == "true" || identifier == "false";
}

void Append(Vector<Span> &spans, const String &text, TokenStyle style)
{
	if (text.empty())
	{
		return;
	}

	if (!spans.empty() && spans.back().style == style)
	{
		spans.back().text += text;
		return;
	}

	spans.push_back(Span{text, style});
}
} // namespace

Vector<Vector<Span>> HighlightSource(const Vector<String> &lines)
{
	Vector<Vector<Span>> highlighted;
	highlighted.reserve(lines.size());

	bool inString = false;
	bool inMultiLineComment = false;

	for (const String &line : lines)
	{
		Vector<Span> spans;

		const std::size_t length = line.size();
		std::size_t cursor = 0;

		while (cursor < length)
		{
			const char character = line[cursor];
			const char next = (cursor + 1) < length ? line[cursor + 1] : '\0';

			if (inMultiLineComment)
			{
				if (character == '*' && next == '/')
				{
					Append(spans, "*/", TokenStyle::Comment);
					cursor += 2;
					inMultiLineComment = false;
					continue;
				}

				Append(spans, String(1, character), TokenStyle::Comment);
				++cursor;
				continue;
			}

			if (inString)
			{
				Append(spans, String(1, character), TokenStyle::Text);
				++cursor;

				if (character == '"')
				{
					inString = false;
				}

				continue;
			}

			if (character == '/' && next == '/')
			{
				Append(spans, line.substr(cursor), TokenStyle::Comment);
				cursor = length;
				continue;
			}

			if (character == '/' && next == '*')
			{
				Append(spans, "/*", TokenStyle::Comment);
				cursor += 2;
				inMultiLineComment = true;
				continue;
			}

			if (character == '"')
			{
				Append(spans, "\"", TokenStyle::Text);
				++cursor;
				inString = true;
				continue;
			}

			if (IsInt(character))
			{
				String number;

				while (cursor < length && IsInt(line[cursor]))
				{
					number += line[cursor];
					++cursor;
				}

				Append(spans, number, TokenStyle::Number);
				continue;
			}

			if (IsAlpha(character))
			{
				String identifier;

				while (cursor < length && IsAlpha(line[cursor]))
				{
					identifier += line[cursor];
					++cursor;
				}

				if (IsBool(identifier))
				{
					Append(spans, identifier, TokenStyle::Boolean);
				}
				else if (IsKeyword(identifier))
				{
					Append(spans, identifier, TokenStyle::Keyword);
				}
				else
				{
					Append(spans, identifier, TokenStyle::Default);
				}

				continue;
			}

			Append(spans, String(1, character), TokenStyle::Default);
			++cursor;
		}

		highlighted.push_back(std::move(spans));
	}

	return highlighted;
}
} // namespace TGX::Shell
