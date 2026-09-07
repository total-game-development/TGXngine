#pragma once

#include <mutex>
#include "Core.h"
#include "Editor.h"
#include "FileSystem.h"
#include "Host.h"
#include "Tasks.h"

namespace TGX::Shell
{
struct Session
{
	String name;
	String user;
	String password;
	FileSystem fileSystem;
};

using ToggleHandler = void (*)(const char *, const char *, bool);

enum class TerminalMode : std::uint8_t
{
	Command,
	Editing
};

class Terminal : public Host
{
private:
	static constexpr std::size_t MAX_OUTPUT = 512;
	static constexpr std::size_t MAX_HISTORY = 64;
	static constexpr unsigned long long RUN_STEP_LIMIT = 5000000ULL;

	Session local;
	Map<String, Session> remotes;
	Session *current = nullptr;

	Vector<String> output;
	Vector<String> history;
	std::size_t historyCursor = 0;

	String input;
	TerminalMode mode = TerminalMode::Command;

	Editor editor;
	TaskPool pool;

	mutable std::recursive_mutex mutex;

	bool requestClose = false;
	ToggleHandler toggleHandler = nullptr;

	void Help();
	void Run(const String &command);
	void Edit(const String &command);
	void Connect(const String &command);
	void Disconnect();
	void Cheat(const String &command);
	void SaveEditor();

	static Vector<String> Split(const String &text, char delimiter);

public:
	Terminal();
	~Terminal() override;

	void Start();
	void Stop();

	void Print(const String &message) override;
	bool ReadFile(const String &name, String &source) override;
	void WriteFile(const String &name, const String &source) override;
	void Toggle(const String &name, const String &value, bool active) override;
	void SetToggleHandler(ToggleHandler handler);
	void Spawn(const String &command) override;

	void Submit(const String &command);
	void Character(char character);
	void Backspace();
	void HistoryUp();
	void HistoryDown();

	void Newline();
	void Tab();
	void CursorUp();
	void CursorDown();
	void CursorLeft();
	void CursorRight();
	void Escape();

	Vector<String> GetOutput() const;
	String GetPrompt() const;
	String GetInput() const;
	TerminalMode GetMode() const;
	Editor &GetEditor();

	bool ShouldClose();

	void Load(const String &path);
	void Save(const String &path) const;
	void Seed();
};
} // namespace TGX::Shell
