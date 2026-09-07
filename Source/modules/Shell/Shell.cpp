#include <algorithm>
#include <mutex>
#include "Logs.h"
#include "Terminal.h"
#include "Window.h"
#include "module_interface.h"

namespace TGX::Shell
{
namespace
{
constexpr unsigned int FONT_SIZE = 18;
constexpr float LINE_HEIGHT = 22.0f;
constexpr float MARGIN_X = 24.0f;
constexpr float MARGIN_Y = 24.0f;

Unique<Terminal> terminal;
String savePath = "Resources/shell.json";

std::size_t VisibleRows()
{
	Window &window = Window::GetInstance();

	const float height = window.GetViewSize().y - (MARGIN_Y * 2.0f);
	const float rows = height / LINE_HEIGHT;

	if (rows < 1.0f)
	{
		return 1;
	}

	return static_cast<std::size_t>(rows);
}

void DrawTerminal()
{
	Window &window = Window::GetInstance();

	const Vector<String> lines = terminal->GetOutput();
	const std::size_t rows = VisibleRows();
	const std::size_t reserved = rows > 0 ? rows - 1 : 0;

	const std::size_t start = lines.size() > reserved ? lines.size() - reserved : 0;

	float y = MARGIN_Y;

	for (std::size_t index = start; index < lines.size(); ++index)
	{
		window.DrawText(lines[index], sf::Vector2f(MARGIN_X, y), FONT_SIZE, sf::Color(190, 230, 190));
		y += LINE_HEIGHT;
	}

	window.DrawText(
		terminal->GetPrompt() + terminal->GetInput() + "_",
		sf::Vector2f(MARGIN_X, y),
		FONT_SIZE,
		sf::Color::White);
}

void DrawEditor()
{
	Window &window = Window::GetInstance();
	Editor &editor = terminal->GetEditor();

	const std::size_t rows = VisibleRows();

	editor.SetVisibleRows(rows > 2 ? rows - 2 : 1);

	window.DrawText(
		"edit " + editor.Name() + (editor.IsDirty() ? " *" : "") + "   [ESC saves and closes]",
		sf::Vector2f(MARGIN_X, MARGIN_Y),
		FONT_SIZE,
		sf::Color(230, 230, 150));

	const Vector<String> &lines = editor.Lines();

	const std::size_t scroll = editor.Scroll();
	const std::size_t limit = std::min(lines.size(), scroll + editor.VisibleRows());

	float y = MARGIN_Y + (LINE_HEIGHT * 2.0f);

	for (std::size_t index = scroll; index < limit; ++index)
	{
		String line = lines[index];

		if (index == editor.Row())
		{
			const std::size_t column = std::min(editor.Column(), line.size());
			line.insert(column, "|");
		}

		window.DrawText(
			std::to_string(index + 1) + "  " + line,
			sf::Vector2f(MARGIN_X, y),
			FONT_SIZE,
			index == editor.Row() ? sf::Color::White : sf::Color(190, 190, 190));

		y += LINE_HEIGHT;
	}
}
} // namespace

extern "C"
{
	MODULE_API void Init()
	{
		Log::Success("Shell initialized");
	}

	MODULE_API void Awake(const String &name)
	{
		if (!terminal)
		{
			terminal = std::make_unique<Terminal>();
		}

		terminal->Load(savePath);
		terminal->Start();

		Log::Success("Shell created: " + name);
	}

	MODULE_API void Create()
	{
		if (terminal)
		{
			terminal->Print("TGXngine shell. Type help for a list of commands.");
		}
	}

	MODULE_API void Update()
	{
	}

	MODULE_API void Draw()
	{
		if (!terminal)
		{
			return;
		}

		if (terminal->GetMode() == TerminalMode::Editing)
		{
			DrawEditor();
			return;
		}

		DrawTerminal();
	}

	MODULE_API void Click()
	{
	}

	MODULE_API void Text(unsigned int codepoint)
	{
		if (!terminal)
		{
			return;
		}

		if (codepoint == '\b')
		{
			terminal->Backspace();
			return;
		}

		if (codepoint == '\r' || codepoint == '\n')
		{
			terminal->Newline();
			return;
		}

		if (codepoint == '\t')
		{
			terminal->Tab();
			return;
		}

		if (codepoint >= 32 && codepoint < 127)
		{
			terminal->Character(static_cast<char>(codepoint));
		}
	}

	MODULE_API void Key(int code)
	{
		if (!terminal)
		{
			return;
		}

		switch (code)
		{
			case static_cast<int>(sf::Keyboard::Up):
				terminal->CursorUp();
				break;
			case static_cast<int>(sf::Keyboard::Down):
				terminal->CursorDown();
				break;
			case static_cast<int>(sf::Keyboard::Left):
				terminal->CursorLeft();
				break;
			case static_cast<int>(sf::Keyboard::Right):
				terminal->CursorRight();
				break;
			case static_cast<int>(sf::Keyboard::Escape):
				terminal->Escape();
				break;
			default:
				break;
		}
	}

	MODULE_API bool ShouldClose()
	{
		return terminal && terminal->ShouldClose();
	}

	MODULE_API void SetToggleHandler(ToggleHandler handler)
	{
		if (terminal)
		{
			terminal->SetToggleHandler(handler);
		}
	}

	MODULE_API void Clear()
	{
		if (terminal)
		{
			terminal->Save(savePath);
			terminal->Stop();
		}
	}

	MODULE_API void Delete()
	{
		if (terminal)
		{
			terminal->Save(savePath);
			terminal->Stop();
			terminal.reset();
		}
	}
}
} // namespace TGX::Shell
