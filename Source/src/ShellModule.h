#pragma once

#include <SFML/Graphics.hpp>
#include "Core.h"

namespace TGX
{
using FNPTR_SHELL_AWAKE = void (*)(const String &);
using FNPTR_SHELL_CREATE = void (*)();
using FNPTR_SHELL_UPDATE = void (*)();
using FNPTR_SHELL_RENDER_WINDOW = void (*)();
using FNPTR_SHELL_CLICK = void (*)();
using FNPTR_SHELL_TEXT = void (*)(unsigned int);
using FNPTR_SHELL_KEY = void (*)(int);
using FNPTR_SHELL_SET_VIEWPORT = void (*)(float, float, float, float);
using FNPTR_SHELL_IS_EDITING = bool (*)();
using FNPTR_SHELL_SHOULD_CLOSE = bool (*)();
using FNPTR_SHELL_SET_TOGGLE_HANDLER = void (*)(void (*)(const char *, const char *, bool));
using FNPTR_SHELL_CLEAR = void (*)();
using FNPTR_SHELL_DELETE = void (*)();

class ShellModule
{
private:
	FNPTR_SHELL_AWAKE awake;
	FNPTR_SHELL_CREATE create;
	FNPTR_SHELL_UPDATE update;
	FNPTR_SHELL_RENDER_WINDOW draw;
	FNPTR_SHELL_CLICK click;
	FNPTR_SHELL_TEXT text;
	FNPTR_SHELL_KEY key;
	FNPTR_SHELL_SET_VIEWPORT setViewport;
	FNPTR_SHELL_IS_EDITING isEditing;
	FNPTR_SHELL_SHOULD_CLOSE shouldClose;
	FNPTR_SHELL_SET_TOGGLE_HANDLER setToggleHandler;
	FNPTR_SHELL_CLEAR clear;
	FNPTR_SHELL_DELETE _delete;

public:
	ShellModule(
		FNPTR_SHELL_AWAKE,
		FNPTR_SHELL_CREATE,
		FNPTR_SHELL_UPDATE,
		FNPTR_SHELL_RENDER_WINDOW,
		FNPTR_SHELL_CLICK,
		FNPTR_SHELL_TEXT,
		FNPTR_SHELL_KEY,
		FNPTR_SHELL_SET_VIEWPORT,
		FNPTR_SHELL_IS_EDITING,
		FNPTR_SHELL_SHOULD_CLOSE,
		FNPTR_SHELL_SET_TOGGLE_HANDLER,
		FNPTR_SHELL_CLEAR,
		FNPTR_SHELL_DELETE);
	~ShellModule() = default;

	void Awake(const String &name);
	void Create();
	void Update();
	void Draw();
	void Click();
	void Text(unsigned int codepoint);
	void Key(int code);
	void SetViewport(const sf::FloatRect &bounds);
	bool IsEditing();
	bool ShouldClose();
	void SetToggleHandler(void (*handler)(const char *, const char *, bool));
	void Clear();
	void Delete();
};
} // namespace TGX
