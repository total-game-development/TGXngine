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
using FNPTR_SHELL_SEND = void (*)(const char *, const char *);
using FNPTR_SHELL_SET_NETWORK = void (*)(FNPTR_SHELL_SEND, const char *);
using FNPTR_SHELL_DELIVER = void (*)(const char *);
using FNPTR_SHELL_SET_CYBER = void (*)(bool, const char *);
using FNPTR_SHELL_LIST_PROCESSES = const char *(*)();
using FNPTR_SHELL_SWITCH_PROCESS = bool (*)(int, bool);
using FNPTR_SHELL_HACK = bool (*)(const char *, bool);
using FNPTR_SHELL_RADAR = const char *(*)();
using FNPTR_SHELL_REVEAL = void (*)(int, int, int);
using FNPTR_SHELL_SET_MATCH_HANDLERS = void (*)(FNPTR_SHELL_LIST_PROCESSES, FNPTR_SHELL_SWITCH_PROCESS, FNPTR_SHELL_HACK, FNPTR_SHELL_RADAR, FNPTR_SHELL_REVEAL);
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
	FNPTR_SHELL_SET_NETWORK setNetwork;
	FNPTR_SHELL_DELIVER deliver;
	FNPTR_SHELL_SET_CYBER setCyber;
	FNPTR_SHELL_SET_MATCH_HANDLERS setMatchHandlers;
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
		FNPTR_SHELL_SET_NETWORK,
		FNPTR_SHELL_DELIVER,
		FNPTR_SHELL_SET_CYBER,
		FNPTR_SHELL_SET_MATCH_HANDLERS,
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
	void SetNetwork(FNPTR_SHELL_SEND send, const String &identity);
	void ClearNetwork();
	void Deliver(const String &message);
	void SetCyber(bool allowed, const String &tutorial);
	void SetMatchHandlers(FNPTR_SHELL_LIST_PROCESSES list, FNPTR_SHELL_SWITCH_PROCESS toggle, FNPTR_SHELL_HACK hack, FNPTR_SHELL_RADAR radar, FNPTR_SHELL_REVEAL reveal);
	void Clear();
	void Delete();
};
} // namespace TGX
