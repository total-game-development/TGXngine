#include <SFML/Graphics/RectangleShape.hpp>
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

sf::FloatRect viewport;

sf::FloatRect Bounds()
{
	if (viewport.width > 0.0f && viewport.height > 0.0f)
	{
		return viewport;
	}

	const sf::Vector2f extent = Window::GetInstance().GetViewSize();

	return {0.0f, 0.0f, extent.x, extent.y};
}

std::size_t VisibleRows()
{
	const float height = Bounds().height - (MARGIN_Y * 2.0f);
	const float rows = height / LINE_HEIGHT;

	if (rows < 1.0f)
	{
		return 1;
	}

	return static_cast<std::size_t>(rows);
}

struct Palette
{
	sf::Color backdrop;
	sf::Color output;
	sf::Color input;
	sf::Color title;
	sf::Color gutter;
	sf::Color keyword;
	sf::Color text;
	sf::Color comment;
	sf::Color number;
	sf::Color boolean;
};

const Palette &CurrentPalette()
{
	static const Palette plain{
		sf::Color::Transparent,
		sf::Color(190, 230, 190),
		sf::Color::White,
		sf::Color(230, 230, 150),
		sf::Color(110, 110, 110),
		sf::Color(0x00, 0xA7, 0xFF),
		sf::Color(0x00, 0xF0, 0x3E),
		sf::Color(0xFF, 0xD4, 0x00),
		sf::Color(0xD5, 0x7C, 0xFF),
		sf::Color(0xFF, 0x70, 0xE9)};

	static const Palette red{
		sf::Color(70, 0, 0, 190),
		sf::Color(255, 170, 160),
		sf::Color(255, 228, 222),
		sf::Color(255, 120, 100),
		sf::Color(150, 75, 75),
		sf::Color(255, 80, 80),
		sf::Color(255, 165, 135),
		sf::Color(255, 125, 60),
		sf::Color(255, 135, 175),
		sf::Color(230, 70, 120)};

	static const Palette blue{
		sf::Color(0, 20, 70, 190),
		sf::Color(165, 200, 255),
		sf::Color(222, 236, 255),
		sf::Color(120, 180, 255),
		sf::Color(75, 95, 150),
		sf::Color(80, 170, 255),
		sf::Color(140, 225, 255),
		sf::Color(125, 135, 255),
		sf::Color(185, 195, 255),
		sf::Color(60, 210, 230)};

	switch (terminal->GetTint())
	{
		case TerminalTint::Red:
			return red;
		case TerminalTint::Blue:
			return blue;
		case TerminalTint::None:
			return plain;
	}

	return plain;
}

sf::Color StyleColour(const Palette &palette, TokenStyle style)
{
	switch (style)
	{
		case TokenStyle::Keyword:
			return palette.keyword;
		case TokenStyle::Text:
			return palette.text;
		case TokenStyle::Comment:
			return palette.comment;
		case TokenStyle::Number:
			return palette.number;
		case TokenStyle::Boolean:
			return palette.boolean;
		case TokenStyle::Default:
			return palette.input;
	}

	return palette.input;
}

void DrawBackdrop(const Palette &palette)
{
	if (palette.backdrop.a == 0)
	{
		return;
	}

	const sf::FloatRect area = Bounds();

	sf::RectangleShape backdrop(sf::Vector2f(area.width, area.height));
	backdrop.setPosition(area.left, area.top);
	backdrop.setFillColor(palette.backdrop);

	Window::GetInstance().Draw(backdrop);
}

void DrawTerminal(const Palette &palette)
{
	Window &window = Window::GetInstance();

	const sf::FloatRect area = Bounds();

	const Vector<String> lines = terminal->GetOutput();
	const std::size_t rows = VisibleRows();
	const std::size_t reserved = rows > 0 ? rows - 1 : 0;

	const std::size_t start = lines.size() > reserved ? lines.size() - reserved : 0;

	const float x = area.left + MARGIN_X;

	float y = area.top + MARGIN_Y;

	for (std::size_t index = start; index < lines.size(); ++index)
	{
		window.DrawText(lines[index], sf::Vector2f(x, y), FONT_SIZE, palette.output);
		y += LINE_HEIGHT;
	}

	window.DrawText(
		terminal->GetPrompt() + terminal->GetInput() + "_",
		sf::Vector2f(x, y),
		FONT_SIZE,
		palette.input);
}

void DrawEditor(const Palette &palette)
{
	Window &window = Window::GetInstance();
	Editor &editor = terminal->GetEditor();

	const sf::FloatRect area = Bounds();

	const std::size_t rows = VisibleRows();

	editor.SetVisibleRows(rows > 2 ? rows - 2 : 1);

	window.DrawText(
		"edit " + editor.Name() + (editor.IsDirty() ? " *" : "") + "   [ESC saves and closes]",
		sf::Vector2f(area.left + MARGIN_X, area.top + MARGIN_Y),
		FONT_SIZE,
		palette.title);

	const Vector<String> &lines = editor.Lines();

	const std::size_t scroll = editor.Scroll();
	const std::size_t limit = std::min(lines.size(), scroll + editor.VisibleRows());

	float y = area.top + MARGIN_Y + (LINE_HEIGHT * 2.0f);

	const Vector<Vector<Span>> &spans = editor.Spans();

	for (std::size_t index = scroll; index < limit; ++index)
	{
		const String gutter = std::to_string(index + 1) + "  ";

		window.DrawText(gutter, sf::Vector2f(area.left + MARGIN_X, y), FONT_SIZE, palette.gutter);

		float x = area.left + MARGIN_X + window.MeasureText(gutter, FONT_SIZE);

		if (index < spans.size())
		{
			for (const Span &span : spans[index])
			{
				window.DrawText(span.text, sf::Vector2f(x, y), FONT_SIZE, StyleColour(palette, span.style));
				x += window.MeasureText(span.text, FONT_SIZE);
			}
		}

		if (index == editor.Row())
		{
			const String &line = lines[index];
			const std::size_t column = std::min(editor.Column(), line.size());
			const float caret = area.left + MARGIN_X + window.MeasureText(gutter + line.substr(0, column), FONT_SIZE);

			window.DrawText("|", sf::Vector2f(caret - 1.0f, y), FONT_SIZE, palette.input);
		}

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
		terminal->SetCyber(true);
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
		if (terminal)
		{
			terminal->Update();
		}
	}

	MODULE_API void Draw()
	{
		if (!terminal)
		{
			return;
		}

		const Palette &palette = CurrentPalette();

		DrawBackdrop(palette);

		if (terminal->GetMode() == TerminalMode::Editing)
		{
			DrawEditor(palette);
			return;
		}

		DrawTerminal(palette);
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

	MODULE_API void SetViewport(float x, float y, float width, float height)
	{
		viewport = {x, y, width, height};
	}

	MODULE_API bool IsEditing()
	{
		return terminal && terminal->GetMode() == TerminalMode::Editing;
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

	MODULE_API void SetNetwork(void (*send)(const char *, const char *), const char *identity)
	{
		if (!terminal)
		{
			return;
		}

		if (send == nullptr || identity == nullptr)
		{
			terminal->ClearNetwork();
			return;
		}

		const nlohmann::json data = nlohmann::json::parse(identity, nullptr, false);

		Vector<String> peers;

		if (data.is_object() && data.contains("machines") && data["machines"].is_array())
		{
			for (const auto &machine : data["machines"])
			{
				if (machine.is_string())
				{
					peers.push_back(machine.get<String>());
				}
			}
		}

		const String self = data.is_object() ? data.value("self", String()) : String();

		RadarSettings radar;

		if (data.is_object() && data.contains("radar") && data["radar"].is_object())
		{
			const nlohmann::json &settings = data["radar"];

			radar.saltDigits = settings.value("saltDigits", radar.saltDigits);
			radar.cell = settings.value("cell", radar.cell);
			radar.publish = std::chrono::milliseconds(static_cast<long long>(settings.value("publishSeconds", 5.0) * 1000.0));
			radar.rotate = std::chrono::milliseconds(static_cast<long long>(settings.value("rotateSeconds", 120.0) * 1000.0));
		}

		terminal->SetRadar(radar);

		terminal->SetNetwork(self, peers, [send](const String &to, const nlohmann::json &body) {
			send(to.c_str(), body.dump().c_str());
		});
	}

	MODULE_API void SetCyber(bool allowed, const char *tutorial)
	{
		if (!terminal)
		{
			return;
		}

		terminal->SetCyber(allowed);

		if (!allowed || tutorial == nullptr)
		{
			return;
		}

		const nlohmann::json data = nlohmann::json::parse(tutorial, nullptr, false);

		if (data.is_object())
		{
			terminal->Tutorial(data.value("directory", String()), data.value("files", nlohmann::json::object()));
		}
	}

	MODULE_API void Deliver(const char *message)
	{
		if (!terminal || message == nullptr)
		{
			return;
		}

		terminal->Deliver(nlohmann::json::parse(message, nullptr, false));
	}

	MODULE_API void SetMatchHandlers(
		const char *(*list)(),
		bool (*toggle)(int, bool),
		bool (*hack)(const char *, bool),
		const char *(*radar)(),
		void (*reveal)(int, int, int))
	{
		if (!terminal)
		{
			return;
		}

		terminal->SetRadarHandlers(
			radar == nullptr ? RadarLister() : RadarLister([radar]() { return nlohmann::json::parse(radar(), nullptr, false); }),
			reveal == nullptr ? RadarReveal() : RadarReveal([reveal](int x, int y, int cell) { reveal(x, y, cell); }));

		if (list == nullptr || toggle == nullptr)
		{
			terminal->SetProcessHandlers(nullptr, nullptr);
		}
		else
		{
			terminal->SetProcessHandlers(
				[list]() { return nlohmann::json::parse(list(), nullptr, false); },
				[toggle](int uid, bool running) { return toggle(uid, running); });
		}

		if (hack == nullptr)
		{
			terminal->SetHackHandler(nullptr);
			return;
		}

		terminal->SetHackHandler([hack](const String &effect, bool cut) { return hack(effect.c_str(), cut); });
	}

	MODULE_API void Clear()
	{
		if (terminal)
		{
			terminal->CommitEditor();
			terminal->ClearNetwork();
			terminal->SetProcessHandlers(nullptr, nullptr);
			terminal->SetRadarHandlers(nullptr, nullptr);
			terminal->Save(savePath);
			terminal->Stop();
		}
	}

	MODULE_API void Delete()
	{
		if (terminal)
		{
			terminal->CommitEditor();
			terminal->ClearNetwork();
			terminal->SetProcessHandlers(nullptr, nullptr);
			terminal->SetRadarHandlers(nullptr, nullptr);
			terminal->Save(savePath);
			terminal->Stop();
			terminal.reset();
		}
	}
}
} // namespace TGX::Shell
