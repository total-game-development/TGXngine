#include "Editor.h"

namespace TGX::Shell
{
Editor::Editor()
{
	lines.push_back(String());
}

void Editor::Open(const String &name, const String &source)
{
	fileName = name;
	lines.clear();

	String current;

	for (const char character : source)
	{
		if (character == '\n')
		{
			lines.push_back(current);
			current.clear();
			continue;
		}

		if (character != '\r')
		{
			current += character;
		}
	}

	lines.push_back(current);

	row = 0;
	column = 0;
	scroll = 0;
	dirty = false;

	Invalidate();
}

void Editor::Close()
{
	fileName.clear();
	lines.clear();
	lines.push_back(String());

	row = 0;
	column = 0;
	scroll = 0;
	dirty = false;

	Invalidate();
}

void Editor::Invalidate()
{
	spansStale = true;
}

const Vector<Vector<Span>> &Editor::Spans() const
{
	if (spansStale)
	{
		spans = HighlightSource(lines);
		spansStale = false;
	}

	return spans;
}

void Editor::ClampColumn()
{
	if (row >= lines.size())
	{
		row = lines.empty() ? 0 : lines.size() - 1;
	}

	if (column > lines[row].size())
	{
		column = lines[row].size();
	}
}

void Editor::Insert(char character)
{
	ClampColumn();

	lines[row].insert(column, 1, character);
	++column;

	dirty = true;
	Invalidate();

	ScrollIntoView();
}

void Editor::Newline()
{
	ClampColumn();

	const String remainder = lines[row].substr(column);
	lines[row].erase(column);

	lines.insert(lines.begin() + static_cast<std::ptrdiff_t>(row) + 1, remainder);

	++row;
	column = 0;

	dirty = true;
	Invalidate();

	ScrollIntoView();
}

void Editor::Backspace()
{
	ClampColumn();

	if (column > 0)
	{
		lines[row].erase(column - 1, 1);
		--column;
		dirty = true;
	Invalidate();

		ScrollIntoView();
		return;
	}

	if (row == 0)
	{
		return;
	}

	const std::size_t previousLength = lines[row - 1].size();

	lines[row - 1] += lines[row];
	lines.erase(lines.begin() + static_cast<std::ptrdiff_t>(row));

	--row;
	column = previousLength;

	dirty = true;
	Invalidate();

	ScrollIntoView();
}

void Editor::Tab()
{
	for (std::size_t index = 0; index < TAB_WIDTH; ++index)
	{
		Insert(' ');
	}
}

void Editor::CursorUp()
{
	if (row > 0)
	{
		--row;
		ClampColumn();
		ScrollIntoView();
	}
}

void Editor::CursorDown()
{
	if (row + 1 < lines.size())
	{
		++row;
		ClampColumn();
		ScrollIntoView();
	}
}

void Editor::CursorLeft()
{
	ClampColumn();

	if (column > 0)
	{
		--column;
		return;
	}

	if (row > 0)
	{
		--row;
		column = lines[row].size();
		ScrollIntoView();
	}
}

void Editor::CursorRight()
{
	ClampColumn();

	if (column < lines[row].size())
	{
		++column;
		return;
	}

	if (row + 1 < lines.size())
	{
		++row;
		column = 0;
		ScrollIntoView();
	}
}

void Editor::Home()
{
	column = 0;
}

void Editor::End()
{
	ClampColumn();
	column = lines[row].size();
}

void Editor::SetVisibleRows(std::size_t rows)
{
	visibleRows = rows == 0 ? 1 : rows;
	ScrollIntoView();
}

void Editor::ScrollIntoView()
{
	if (row < scroll)
	{
		scroll = row;
		return;
	}

	if (row >= scroll + visibleRows)
	{
		scroll = row - visibleRows + 1;
	}
}

String Editor::Source() const
{
	String source;

	for (std::size_t index = 0; index < lines.size(); ++index)
	{
		if (index > 0)
		{
			source += "\n";
		}

		source += lines[index];
	}

	return source;
}

const String &Editor::Name() const
{
	return fileName;
}

const Vector<String> &Editor::Lines() const
{
	return lines;
}

std::size_t Editor::Row() const
{
	return row;
}

std::size_t Editor::Column() const
{
	return column;
}

std::size_t Editor::Scroll() const
{
	return scroll;
}

std::size_t Editor::VisibleRows() const
{
	return visibleRows;
}

bool Editor::IsDirty() const
{
	return dirty;
}

void Editor::ClearDirty()
{
	dirty = false;
}
} // namespace TGX::Shell
