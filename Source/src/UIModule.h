#pragma once

#include <SFML/Graphics.hpp>
#include "Core.h"

namespace TGX
{
using FNPTR_UI_AWAKE = void (*)(const String &);
using FNPTR_UI_CREATE = void (*)();
using FNPTR_UI_UPDATE = void (*)();
using FNPTR_UI_RENDER_WINDOW = void (*)();
using FNPTR_UI_CLICK = bool (*)();
using FNPTR_UI_RELEASE = void (*)();
using FNPTR_UI_TEXT = bool (*)(unsigned int);
using FNPTR_UI_KEY = bool (*)(int);
using FNPTR_UI_CONSOLE_BOUNDS = bool (*)(float *, float *, float *, float *);
using FNPTR_UI_IS_VISIBLE = bool (*)();
using FNPTR_UI_IS_PAUSED = bool (*)();
using FNPTR_UI_CLEAR = void (*)();
using FNPTR_UI_DELETE = void (*)();

class UIModule
{
private:
	FNPTR_UI_AWAKE awake;
	FNPTR_UI_CREATE create;
	FNPTR_UI_UPDATE update;
	FNPTR_UI_RENDER_WINDOW draw;
	FNPTR_UI_CLICK click;
	FNPTR_UI_RELEASE release;
	FNPTR_UI_TEXT text;
	FNPTR_UI_KEY key;
	FNPTR_UI_CONSOLE_BOUNDS consoleBounds;
	FNPTR_UI_IS_VISIBLE isVisible;
	FNPTR_UI_IS_PAUSED isPaused;
	FNPTR_UI_CLEAR clear;
	FNPTR_UI_DELETE _delete;

public:
	UIModule(
		FNPTR_UI_AWAKE,
		FNPTR_UI_CREATE,
		FNPTR_UI_UPDATE,
		FNPTR_UI_RENDER_WINDOW,
		FNPTR_UI_CLICK,
		FNPTR_UI_RELEASE,
		FNPTR_UI_TEXT,
		FNPTR_UI_KEY,
		FNPTR_UI_CONSOLE_BOUNDS,
		FNPTR_UI_IS_VISIBLE,
		FNPTR_UI_IS_PAUSED,
		FNPTR_UI_CLEAR,
		FNPTR_UI_DELETE);
	~UIModule() = default;

	void Awake(const String &name);
	void Create();
	void Update();
	void Draw();
	bool Click();
	void Release();
	bool Text(unsigned int codepoint);
	bool Key(int code);
	bool ConsoleBounds(sf::FloatRect &bounds);
	bool IsVisible();
	bool IsPaused();
	void Clear();
	void Delete();
};
} // namespace TGX
