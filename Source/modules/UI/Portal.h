#pragma once

#include <SFML/Graphics.hpp>
#include "Page.h"
#include "Panel.h"
#include "Widget.h"

namespace TGX::UI
{
struct Screen
{
	String name;
	bool pause = false;
	Vector<Element> elements;
};

int KeyFromString(const String &name);

class Portal
{
private:
	Map<String, Screen> screens;
	Map<String, Page> pages;

	Vector<Unique<Widget>> widgets;
	Vector<Action> pending;
	Set<String> windowRegistry;

	String screenName;
	String home = "home";
	String console = "shell";

	String iconPath = "Resources/images/ui/icons/";
	String windowPath = "Resources/images/ui/windows/";
	String imagePath = "Resources/images/ui/images/";

	Vector<int> toggles = {static_cast<int>(sf::Keyboard::Tab)};

	sf::Color backdrop = sf::Color(0x05, 0x07, 0x0A, 0xF2);

	bool scanlines = true;

	bool visible = false;
	bool loaded = false;
	bool startVisible = false;

	sf::Vector2f view;

	Widget *focus = nullptr;

	void Build();
	void DrawBackdrop();
	void Determine(const Element &element);
	void OpenWindow(const Element &element);
	void CloseWindow(const String &name);
	void Attach(Unique<Widget> widget);
	void Sweep();
	void Perform(const Action &action);
	void Execute(const Action &action);
	void Flush();
	const Screen *Current() const;

public:
	void Load(const String &path);
	void Awake(const String &name);
	void Start();

	void Show(const String &name);
	void Hide();
	void ToggleVisible();

	void Update();
	void Draw();

	bool Press();
	void Release();
	bool Character(unsigned int codepoint);
	bool Key(int code);

	bool Console(sf::FloatRect &bounds) const;

	bool IsVisible() const;
	bool IsPaused() const;
	bool IsLoaded() const;

	void Clear();
};
} // namespace TGX::UI
