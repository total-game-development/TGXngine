#pragma once

#include <gtest/gtest.h>
#include "Highlight.h"

namespace TGX::Shell
{
inline TokenStyle StyleOf(const Vector<Span> &spans, const String &text)
{
	for (const Span &span : spans)
	{
		if (span.text == text)
		{
			return span.style;
		}
	}

	return TokenStyle::Default;
}

inline String Rebuild(const Vector<Span> &spans)
{
	String line;

	for (const Span &span : spans)
	{
		line += span.text;
	}

	return line;
}

TEST(ShellHighlight, ColoursKeywords)
{
	const Vector<Vector<Span>> spans = HighlightSource({"let x = 1"});

	ASSERT_EQ(spans.size(), 1u);
	EXPECT_EQ(StyleOf(spans[0], "let"), TokenStyle::Keyword);
}

TEST(ShellHighlight, ColoursNumbers)
{
	const Vector<Vector<Span>> spans = HighlightSource({"let x = 42"});

	EXPECT_EQ(StyleOf(spans[0], "42"), TokenStyle::Number);
}

TEST(ShellHighlight, ColoursBooleans)
{
	const Vector<Vector<Span>> spans = HighlightSource({"let x = true"});

	EXPECT_EQ(StyleOf(spans[0], "true"), TokenStyle::Boolean);
}

TEST(ShellHighlight, LeavesIdentifiersDefault)
{
	const Vector<Vector<Span>> spans = HighlightSource({"let counter = 1"});

	EXPECT_EQ(StyleOf(spans[0], "counter"), TokenStyle::Default);
}

TEST(ShellHighlight, ColoursStringsIncludingQuotes)
{
	const Vector<Vector<Span>> spans = HighlightSource({"print(\"hi\")"});

	ASSERT_EQ(spans.size(), 1u);

	bool sawText = false;

	for (const Span &span : spans[0])
	{
		if (span.style == TokenStyle::Text)
		{
			EXPECT_NE(span.text.find('"'), String::npos);
			sawText = true;
		}
	}

	EXPECT_TRUE(sawText);
}

TEST(ShellHighlight, DoesNotColourKeywordsInsideStrings)
{
	const Vector<Vector<Span>> spans = HighlightSource({"print(\"let me in\")"});

	for (const Span &span : spans[0])
	{
		if (span.text.find("let") != String::npos)
		{
			EXPECT_NE(span.style, TokenStyle::Keyword);
		}
	}
}

TEST(ShellHighlight, ColoursLineComments)
{
	const Vector<Vector<Span>> spans = HighlightSource({"let x = 1 // trailing"});

	bool sawComment = false;

	for (const Span &span : spans[0])
	{
		if (span.style == TokenStyle::Comment)
		{
			EXPECT_NE(span.text.find("trailing"), String::npos);
			sawComment = true;
		}
	}

	EXPECT_TRUE(sawComment);
}

TEST(ShellHighlight, CarriesBlockCommentsAcrossLines)
{
	const Vector<Vector<Span>> spans = HighlightSource({"/* start", "still inside", "end */ let x = 1"});

	ASSERT_EQ(spans.size(), 3u);

	ASSERT_EQ(spans[1].size(), 1u);
	EXPECT_EQ(spans[1][0].style, TokenStyle::Comment);

	EXPECT_EQ(StyleOf(spans[2], "let"), TokenStyle::Keyword);
}

TEST(ShellHighlight, CarriesUnterminatedStringsAcrossLines)
{
	const Vector<Vector<Span>> spans = HighlightSource({"let a = \"open", "let b = 1"});

	ASSERT_EQ(spans.size(), 2u);
	EXPECT_NE(StyleOf(spans[1], "let"), TokenStyle::Keyword);
}

TEST(ShellHighlight, PreservesEveryCharacterOfTheLine)
{
	const Vector<String> lines = {"function add(a, b) { return a + b } // sums", "print(add(1, 2))"};

	const Vector<Vector<Span>> spans = HighlightSource(lines);

	ASSERT_EQ(spans.size(), lines.size());

	for (std::size_t index = 0; index < lines.size(); ++index)
	{
		EXPECT_EQ(Rebuild(spans[index]), lines[index]);
	}
}

TEST(ShellHighlight, HandlesEmptyLines)
{
	const Vector<Vector<Span>> spans = HighlightSource({"", "let x = 1", ""});

	ASSERT_EQ(spans.size(), 3u);
	EXPECT_TRUE(spans[0].empty());
	EXPECT_TRUE(spans[2].empty());
}
} // namespace TGX::Shell
