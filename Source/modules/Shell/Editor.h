#pragma once

#include <cstdint>
#include "Core.h"
#include "Highlight.h"

namespace TGX::Shell
{
class Editor
{
private:
	static constexpr std::size_t TAB_WIDTH = 4;

	String fileName;
	Vector<String> lines;

	std::size_t row = 0;
	std::size_t column = 0;
	std::size_t scroll = 0;
	std::size_t visibleRows = 24;

	bool dirty = false;

	mutable Vector<Vector<Span>> spans;
	mutable bool spansStale = true;

	void ClampColumn();
	void Invalidate();

public:
	Editor();

	void Open(const String &name, const String &source);
	void Close();

	void Insert(char character);
	void Newline();
	void Backspace();
	void Tab();

	void CursorUp();
	void CursorDown();
	void CursorLeft();
	void CursorRight();
	void Home();
	void End();

	void SetVisibleRows(std::size_t rows);
	void ScrollIntoView();

	String Source() const;
	const String &Name() const;
	const Vector<String> &Lines() const;
	const Vector<Vector<Span>> &Spans() const;

	std::size_t Row() const;
	std::size_t Column() const;
	std::size_t Scroll() const;
	std::size_t VisibleRows() const;

	bool IsDirty() const;
	void ClearDirty();
};
} // namespace TGX::Shell
