#include "Portal.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <utility>
#include "Enums.h"
#include "Layout.h"
#include "Logs.h"
#include "Textures.h"
#include "Window.h"
#include "WorldState.h"

namespace TGX::UI
{
namespace
{
const Map<String, int> KEYS = {
	{"Tab", static_cast<int>(sf::Keyboard::Tab)},
	{"Escape", static_cast<int>(sf::Keyboard::Escape)},
	{"Space", static_cast<int>(sf::Keyboard::Space)},
	{"Enter", static_cast<int>(sf::Keyboard::Enter)},
	{"BackSpace", static_cast<int>(sf::Keyboard::BackSpace)},
	{"Tilde", static_cast<int>(sf::Keyboard::Tilde)},
	{"End", static_cast<int>(sf::Keyboard::End)},
	{"Home", static_cast<int>(sf::Keyboard::Home)},
	{"F1", static_cast<int>(sf::Keyboard::F1)},
	{"F2", static_cast<int>(sf::Keyboard::F2)},
	{"F3", static_cast<int>(sf::Keyboard::F3)},
	{"F4", static_cast<int>(sf::Keyboard::F4)},
	{"F5", static_cast<int>(sf::Keyboard::F5)},
	{"F6", static_cast<int>(sf::Keyboard::F6)},
	{"F7", static_cast<int>(sf::Keyboard::F7)},
	{"F8", static_cast<int>(sf::Keyboard::F8)},
	{"F9", static_cast<int>(sf::Keyboard::F9)},
	{"F10", static_cast<int>(sf::Keyboard::F10)},
	{"F11", static_cast<int>(sf::Keyboard::F11)},
	{"F12", static_cast<int>(sf::Keyboard::F12)}};
} // namespace

int KeyFromString(const String &name)
{
	const auto found = KEYS.find(name);

	if (found != KEYS.end())
	{
		return found->second;
	}

	if (name.size() == 1 && name[0] >= 'A' && name[0] <= 'Z')
	{
		return static_cast<int>(sf::Keyboard::A) + (name[0] - 'A');
	}

	Log::Warning("UI toggle key not understood: " + name);

	return static_cast<int>(sf::Keyboard::Tab);
}

void Portal::Load(const String &path)
{
	if (!std::filesystem::exists(path))
	{
		Log::Error("UI configuration file missing: " + path);
		return;
	}

	json source;
	std::ifstream stream(path);

	if (!(stream >> source))
	{
		Log::Error("Failed to parse UI JSON: " + path);
		return;
	}

	if (!source.contains("ui") || !source["ui"].is_object())
	{
		Log::Error("UI JSON has no 'ui' object: " + path);
		return;
	}

	const json &root = source["ui"];

	if (root.contains("paths") && root["paths"].is_object())
	{
		const json &paths = root["paths"];

		iconPath = paths.value("icons", iconPath);
		windowPath = paths.value("windows", windowPath);
		imagePath = paths.value("images", imagePath);
	}

	if (root.contains("backdrop") && root["backdrop"].is_object())
	{
		const json &shade = root["backdrop"];

		backdrop = ColourFromJson(shade, "colour", backdrop);
		backdrop.a = static_cast<std::uint8_t>(shade.value("alpha", static_cast<int>(backdrop.a)));
		scanlines = shade.value("scanlines", scanlines);
	}

	toggles.clear();

	if (root.contains("toggle") && root["toggle"].is_array())
	{
		for (const auto &entry : root["toggle"])
		{
			toggles.push_back(KeyFromString(entry.get<String>()));
		}
	}
	else
	{
		toggles.push_back(KeyFromString(root.value("toggle", String{"Tab"})));
	}

	if (toggles.empty())
	{
		toggles.push_back(static_cast<int>(sf::Keyboard::Tab));
	}

	home = root.value("screen", home);
	console = root.value("console", console);
	startVisible = root.value("visible", startVisible);

	if (root.contains("screens") && root["screens"].is_object())
	{
		for (const auto &[name, entry] : root["screens"].items())
		{
			Screen screen;

			screen.name = name;

			if (entry.is_array())
			{
				screen.elements = ElementsFromJson(entry);
			}
			else if (entry.is_object())
			{
				screen.pause = entry.value("pause", false);
				screen.elements = ElementsFromJson(entry["elements"]);
			}

			screens[name] = std::move(screen);
		}
	}

	if (root.contains("pages"))
	{
		pages = PagesFromJson(root["pages"]);
	}

	loaded = !screens.empty();

	if (!loaded)
	{
		Log::Error("UI JSON declares no screens: " + path);
		return;
	}

	Log::Success("UI loaded " + std::to_string(screens.size()) + " screens and " + std::to_string(pages.size()) + " pages");
}

void Portal::Awake(const String &name)
{
	if (!name.empty() && screens.contains(name))
	{
		home = name;
	}

	screenName = home;
}

void Portal::Start()
{
	if (!loaded)
	{
		return;
	}

	if (startVisible)
	{
		Show(home);
		return;
	}

	screenName = home;
}

const Screen *Portal::Current() const
{
	const auto found = screens.find(screenName);

	return found == screens.end() ? nullptr : &found->second;
}

void Portal::Build()
{
	widgets.clear();
	windowRegistry.clear();

	focus = nullptr;

	const Screen *screen = Current();

	if (screen == nullptr)
	{
		Log::Error("UI screen not found: " + screenName);
		return;
	}

	for (const Element &element : screen->elements)
	{
		Determine(element);
	}

	Log::Info("UI screen built: " + screenName);
}

void Portal::Determine(const Element &element)
{
	switch (element.type)
	{
		case ElementType::Container:
			for (const Element &child : element.elements)
			{
				Determine(child);
			}
			break;

		case ElementType::Text:
			Attach(std::make_unique<Label>(element));
			break;

		case ElementType::Button:
			Attach(std::make_unique<Button>(element));
			break;

		case ElementType::IconButton:
			{
				Element icon = element;

				if (icon.path.empty())
				{
					icon.path = iconPath;
				}

				Attach(std::make_unique<IconButton>(icon));
				break;
			}

		case ElementType::TextInput:
			Attach(std::make_unique<TextField>(element));
			break;

		case ElementType::Window:
			OpenWindow(element);
			break;

		default:
			break;
	}
}

void Portal::OpenWindow(const Element &element)
{
	const String key = element.title.empty() ? element.name : element.title;

	if (key.empty())
	{
		Log::Warning("UI window has neither a title nor a name");
		return;
	}

	if (windowRegistry.contains(key))
	{
		return;
	}

	Page page;

	const auto found = pages.find(key);

	if (found != pages.end())
	{
		page = found->second;
	}
	else
	{
		Log::Warning("UI page not found, opening an empty window: " + key);

		page.key = key;
		page.title = key;
	}

	if (!element.name.empty() && found == pages.end())
	{
		page.window = element.name;
	}

	windowRegistry.insert(key);

	auto panel = std::make_unique<Panel>(element, page, windowPath, imagePath);

	panel->SetCascade(static_cast<int>(windowRegistry.size()) - 1);

	Attach(std::move(panel));

	Log::Info("UI window opened: " + key);
}

void Portal::CloseWindow(const String &name)
{
	for (auto &widget : widgets)
	{
		auto *panel = dynamic_cast<Panel *>(widget.get());

		if (panel != nullptr && panel->Identity() == name)
		{
			panel->Dismiss();
		}
	}

	Sweep();
}

void Portal::Attach(Unique<Widget> widget)
{
	widget->SetHandler([this](const Action &action) { Perform(action); });

	view = Window::GetInstance().GetViewSize();

	widget->Arrange(view);

	widgets.push_back(std::move(widget));
}

void Portal::Sweep()
{
	for (auto it = widgets.begin(); it != widgets.end();)
	{
		if (!(*it)->Dismissed())
		{
			++it;
			continue;
		}

		windowRegistry.erase((*it)->Identity());

		if (focus == it->get())
		{
			focus = nullptr;
		}

		it = widgets.erase(it);
	}
}

void Portal::Perform(const Action &action)
{
	pending.push_back(action);
}

void Portal::Flush()
{
	while (!pending.empty())
	{
		const Action action = pending.front();

		pending.erase(pending.begin());

		Execute(action);
	}
}

void Portal::Execute(const Action &action)
{
	WorldState &world = WorldState::GetInstance();

	if (action.condition == "gotoScreen")
	{
		Show(action.value);
		return;
	}

	if (action.condition == "openWindow")
	{
		Element window;

		window.type = ElementType::Window;
		window.title = action.value;

		Determine(window);
		return;
	}

	if (action.condition == "closeWindow")
	{
		CloseWindow(action.value);
		return;
	}

	if (action.condition == "closeUI" || action.condition == "resume")
	{
		Hide();
		return;
	}

	if (action.condition == "loadScene")
	{
		Hide();

		world.gameEvents.emplace_back(UIAction::LoadScene, action.value);
		return;
	}

	if (action.condition == "print")
	{
		world.gameEvents.emplace_back(UIAction::Print, action.value);
		return;
	}

	if (action.condition == "quit")
	{
		world.SetClosed(true);
		return;
	}

	Log::Warning("UI action not understood: " + action.condition);
}

void Portal::Show(const String &name)
{
	if (!screens.contains(name))
	{
		Log::Error("UI screen not found: " + name);
		return;
	}

	screenName = name;
	visible = true;

	Build();
}

void Portal::Hide()
{
	visible = false;

	widgets.clear();
	windowRegistry.clear();
	pending.clear();

	focus = nullptr;
}

void Portal::ToggleVisible()
{
	if (visible)
	{
		Hide();
		return;
	}

	Show(screenName.empty() ? home : screenName);
}

void Portal::Update()
{
	if (!visible)
	{
		return;
	}

	Window &window = Window::GetInstance();

	const sf::Vector2f current = window.GetViewSize();

	if (current != view)
	{
		view = current;

		for (auto &widget : widgets)
		{
			widget->Arrange(view);
		}
	}

	const sf::Vector2f point = window.GetMousePosition();

	for (auto &widget : widgets)
	{
		widget->Move(point);
		widget->Update();
	}

	Flush();
	Sweep();
}

void Portal::DrawBackdrop()
{
	Window &window = Window::GetInstance();

	const sf::Vector2f extent = window.GetViewSize();

	sf::RectangleShape shade(extent);
	shade.setFillColor(backdrop);

	window.Draw(shade);

	if (!scanlines)
	{
		return;
	}

	sf::RectangleShape line({extent.x, 1.0f});
	line.setFillColor(sf::Color(0x00, 0xA7, 0xFF, 0x0E));

	for (float y = 0.0f; y < extent.y; y += 4.0f)
	{
		line.setPosition(0.0f, y);

		window.Draw(line);
	}
}

void Portal::Draw()
{
	if (!visible)
	{
		return;
	}

	DrawBackdrop();

	for (auto &widget : widgets)
	{
		widget->Draw();
	}
}

bool Portal::Press()
{
	if (!visible)
	{
		return false;
	}

	const sf::Vector2f point = Window::GetInstance().GetMousePosition();

	bool consumed = false;

	for (std::size_t index = widgets.size(); index > 0; index--)
	{
		Unique<Widget> &widget = widgets[index - 1];

		if (!widget->Press(point))
		{
			continue;
		}

		if (focus != nullptr && focus != widget.get())
		{
			focus->SetFocused(false);
			focus = nullptr;
		}

		if (widget->WantsFocus())
		{
			focus = widget.get();
			focus->SetFocused(true);
		}

		if (dynamic_cast<Panel *>(widget.get()) != nullptr && index != widgets.size())
		{
			Unique<Widget> raised = std::move(widgets[index - 1]);

			widgets.erase(widgets.begin() + static_cast<long>(index - 1));
			widgets.push_back(std::move(raised));
		}

		consumed = true;
		break;
	}

	if (!consumed && focus != nullptr)
	{
		focus->SetFocused(false);
		focus = nullptr;
	}

	Flush();
	Sweep();

	return consumed || IsPaused();
}

void Portal::Release()
{
	for (auto &widget : widgets)
	{
		widget->Release();
	}
}

bool Portal::Character(unsigned int codepoint)
{
	if (!visible || focus == nullptr)
	{
		return false;
	}

	const bool handled = focus->Character(codepoint);

	Flush();

	return handled;
}

bool Portal::Key(int code)
{
	sf::FloatRect bounds;

	const bool terminal = Console(bounds);

	if (std::find(toggles.begin(), toggles.end(), code) != toggles.end())
	{
		// An open console has first claim on Tab, which completes a path there.
		// Every other toggle still puts the portal away from inside it.
		if (!terminal || code != static_cast<int>(sf::Keyboard::Tab))
		{
			ToggleVisible();

			return true;
		}
	}

	if (!visible)
	{
		return false;
	}

	if (focus != nullptr && focus->Key(code))
	{
		Flush();

		return true;
	}

	for (std::size_t index = widgets.size(); index > 0; index--)
	{
		if (dynamic_cast<Panel *>(widgets[index - 1].get()) == nullptr)
		{
			continue;
		}

		const bool handled = widgets[index - 1]->Key(code);

		Sweep();

		if (handled)
		{
			return true;
		}

		break;
	}

	if (code == static_cast<int>(sf::Keyboard::Escape))
	{
		Hide();

		return true;
	}

	// A key the portal has no use for belongs to the console when one is open,
	// so it is reported unhandled rather than swallowed by the pause.
	return terminal ? false : IsPaused();
}

bool Portal::Console(sf::FloatRect &bounds) const
{
	if (!visible)
	{
		return false;
	}

	for (const auto &widget : widgets)
	{
		auto *panel = dynamic_cast<Panel *>(widget.get());

		if (panel == nullptr || panel->Identity() != console || panel->Dismissed())
		{
			continue;
		}

		bounds = panel->Content();

		return true;
	}

	return false;
}

bool Portal::IsVisible() const
{
	return visible;
}

bool Portal::IsPaused() const
{
	if (!visible)
	{
		return false;
	}

	const Screen *screen = Current();

	return screen != nullptr && screen->pause;
}

bool Portal::IsLoaded() const
{
	return loaded;
}

void Portal::Clear()
{
	Hide();

	screens.clear();
	pages.clear();

	loaded = false;

	Textures::Clear();
}
} // namespace TGX::UI
