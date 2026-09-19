#pragma once

#include <chrono>
#include <map>
#include <mutex>
#include <nlohmann/json.hpp>
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

	bool networked = false;
	String directory = "/";
};

using ToggleHandler = void (*)(const char *, const char *, bool);
using NetworkSender = Function<void(const String &, const nlohmann::json &)>;
using ProcessLister = Function<nlohmann::json()>;
using ProcessSwitch = Function<bool(int, bool)>;

enum class TerminalMode : std::uint8_t
{
	Command,
	Editing
};

class Terminal : public Host
{
private:
	using Clock = std::chrono::steady_clock;

	struct Request
	{
		String machine;
		String op;
		String name;
		String directory;
		Clock::time_point sent;
	};

	struct EditTarget
	{
		String machine;
		String directory;
	};

	static constexpr std::size_t MAX_OUTPUT = 512;
	static constexpr std::size_t MAX_HISTORY = 64;
	static constexpr unsigned long long RUN_STEP_LIMIT = 5000000ULL;
	static constexpr std::size_t MAX_REMOTE_SOURCE = 32 * 1024;
	static constexpr std::chrono::milliseconds REQUEST_TIMEOUT{5000};
	static constexpr std::chrono::milliseconds PROCESS_REFRESH{250};

	Session local;
	Map<String, Session> remotes;
	Map<String, Session> machines;
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
	bool showFogOfWar = false;
	String savePath;
	ToggleHandler toggleHandler = nullptr;

	NetworkSender sender;
	String self;
	bool multiplayer = false;
	int nextRequest = 0;
	std::map<int, Request> requests;
	Optional<EditTarget> editTarget;

	ProcessLister processLister;
	ProcessSwitch processSwitch;
	nlohmann::json processes = nlohmann::json::object();
	std::map<int, int> buildingPids;
	int nextPid = 1;
	Clock::time_point refreshed;

	void Help();
	void Run(const String &command);
	void Edit(const String &command);
	void Connect(const String &command);
	void Disconnect();
	void Hosts();
	void Cheat(const String &command);
	void SaveEditor();
	void Persist();

	FileSystem &Files();

	bool Remote(const String &head, const Vector<String> &args);
	void Send(const String &machine, const String &directory, const String &op, const Vector<String> &args, const String &source = String());
	void Answer(const String &from, const nlohmann::json &body);
	void Receive(const String &from, const nlohmann::json &body);
	void Refused(const nlohmann::json &message);
	void Abandon(const Request &request, const String &reason);

	void RefreshProcesses();
	void ListProcesses();
	void Signal(const Vector<String> &args, bool start);

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

	void SetNetwork(const String &name, const Vector<String> &peers, NetworkSender send);
	void ClearNetwork();
	void Deliver(const nlohmann::json &message);

	void SetProcessHandlers(ProcessLister lister, ProcessSwitch switcher);

	void Update();

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
	void CommitEditor();
	void Save(const String &path) const;
	void Seed();
};
} // namespace TGX::Shell
