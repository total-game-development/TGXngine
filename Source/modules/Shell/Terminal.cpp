#include "Terminal.h"
#include <algorithm>
#include <fstream>
#include <random>
#include "Interpreter.h"

namespace TGX::Shell
{
namespace
{
String Trim(const String &text)
{
	std::size_t start = 0;
	std::size_t end = text.size();

	while (start < end && (text[start] == ' ' || text[start] == '\t' || text[start] == '\r'))
	{
		++start;
	}

	while (end > start && (text[end - 1] == ' ' || text[end - 1] == '\t' || text[end - 1] == '\r'))
	{
		--end;
	}

	return text.substr(start, end - start);
}

const Map<String, TerminalTint> &Tints()
{
	static const Map<String, TerminalTint> tints = {
		{"off", TerminalTint::None},
		{"cyber red", TerminalTint::Red},
		{"cyber blue", TerminalTint::Blue}};

	return tints;
}

String TintName(TerminalTint tint)
{
	for (const auto &entry : Tints())
	{
		if (entry.second == tint)
		{
			return entry.first;
		}
	}

	return "off";
}
} // namespace

Terminal::Terminal()
{
	local.name = "local";
	local.user = "naomi";
	local.password = Pin();
	current = &local;
}

String Terminal::Digits(int count)
{
	static std::mt19937 source{std::random_device{}()};

	std::uniform_int_distribution<int> digit(0, 9);

	String digits;

	for (int index = 0; index < count; ++index)
	{
		digits += static_cast<char>('0' + digit(source));
	}

	return digits;
}

String Terminal::Pin()
{
	return Digits(4);
}

Terminal::~Terminal()
{
	Stop();
}

void Terminal::Start()
{
	pool.Start(this);
}

void Terminal::Stop()
{
	pool.Stop();
}

Vector<String> Terminal::Split(const String &text, char delimiter)
{
	Vector<String> parts;
	String current;

	for (const char character : text)
	{
		if (character == delimiter)
		{
			if (!current.empty())
			{
				parts.push_back(current);
				current.clear();
			}

			continue;
		}

		current += character;
	}

	if (!current.empty())
	{
		parts.push_back(current);
	}

	return parts;
}

void Terminal::Print(const String &message)
{
	std::lock_guard<std::recursive_mutex> lock(mutex);

	if (serving != nullptr)
	{
		serving->push_back(message);
	}

	output.push_back(message);

	while (output.size() > MAX_OUTPUT)
	{
		output.erase(output.begin());
	}
}

FileSystem &Terminal::Files()
{
	return current->networked ? local.fileSystem : current->fileSystem;
}

bool Terminal::ReadFile(const String &name, String &source)
{
	std::lock_guard<std::recursive_mutex> lock(mutex);

	return Files().Read(name, source);
}

void Terminal::WriteFile(const String &name, const String &source)
{
	std::lock_guard<std::recursive_mutex> lock(mutex);

	Files().Write(name, source);

	Persist();
}

void Terminal::SetToggleHandler(ToggleHandler handler)
{
	std::lock_guard<std::recursive_mutex> lock(mutex);

	toggleHandler = handler;
}

void Terminal::Toggle(const String &name, const String &value, bool active)
{
	ToggleHandler handler = nullptr;

	{
		std::lock_guard<std::recursive_mutex> lock(mutex);
		handler = toggleHandler;
	}

	if (serving != nullptr)
	{
		Print("Toggles are refused from a remote session");
		return;
	}

	if (handler == nullptr)
	{
		Print("Unknown toggle: " + name + " " + value);
		return;
	}

	handler(name.c_str(), value.c_str(), active);
}

void Terminal::Spawn(const String &command)
{
	String source;

	const Vector<String> args = Split(command, ' ');

	if (args.empty())
	{
		Print("Invalid command. Usage: spawn <fileName>");
		return;
	}

	{
		std::lock_guard<std::recursive_mutex> lock(mutex);

		if (!Files().Read(args[0], source))
		{
			Print("File " + args[0] + " doesn't exist");
			return;
		}
	}

	Parser parser;
	NodeRef program = parser.Produce(source);

	if (!parser.GetErrors().empty())
	{
		for (const String &error : parser.GetErrors())
		{
			Print(error);
		}

		return;
	}

	Task task;
	task.name = args[0];
	task.program = program;
	task.args = Vector<String>(args.begin() + 1, args.end());

	{
		std::lock_guard<std::recursive_mutex> lock(mutex);
		task.pid = nextPid++;
	}

	pool.Enqueue(std::move(task));
}

void Terminal::Submit(const String &line)
{
	const String typed = Trim(line);

	Print(GetPrompt() + typed);

	if (!typed.empty())
	{
		history.push_back(typed);

		while (history.size() > MAX_HISTORY)
		{
			history.erase(history.begin());
		}
	}

	historyCursor = history.size();

	if (typed.empty())
	{
		return;
	}

	const String command = typed.rfind("./", 0) == 0 ? "run " + typed.substr(2) : typed;
	const Vector<String> args = Split(command, ' ');
	const String head = args.empty() ? String() : args[0];

	if (Refuses(head))
	{
		return;
	}

	if (current->networked && Remote(head, args))
	{
		return;
	}

	String message;

	if (head == "help")
	{
		Help();
	}
	else if (head == "clear" || head == "cls")
	{
		std::lock_guard<std::recursive_mutex> lock(mutex);
		output.clear();
	}
	else if (head == "run")
	{
		Run(command);
	}
	else if (head == "edit" || head == "ed")
	{
		Edit(command);
	}
	else if (head == "changedir" || head == "cd")
	{
		if (args.size() != 2)
		{
			Print("Invalid command. Usage: " + head + " <directory>");
		}
		else
		{
			current->fileSystem.ChangeDirectory(args[1], message);
			Print(message);
		}
	}
	else if (head == "makedir" || head == "mkdir")
	{
		if (args.size() != 2)
		{
			Print("Invalid command. Usage: " + head + " <directory>");
		}
		else
		{
			current->fileSystem.MakeDirectory(args[1], message);
			Print(message);
			Persist();
		}
	}
	else if (head == "make" || head == "mk")
	{
		if (args.size() != 2)
		{
			Print("Invalid command. Usage: " + head + " <fileName>");
		}
		else
		{
			current->fileSystem.MakeFile(args[1], message);
			Print(message);
			Persist();
		}
	}
	else if (head == "delete" || head == "del")
	{
		if (args.size() != 2)
		{
			Print("Invalid command. Usage: " + head + " <fileName>");
		}
		else
		{
			current->fileSystem.Remove(args[1], message);
			Print(message);
			Persist();
		}
	}
	else if (head == "rename" || head == "rn")
	{
		if (args.size() != 3)
		{
			Print("Invalid command. Usage: " + head + " <fileName> <newName>");
		}
		else
		{
			current->fileSystem.Rename(args[1], args[2], message);
			Print(message);
			Persist();
		}
	}
	else if (head == "print" || head == "pwd")
	{
		Print(current->fileSystem.GetCurrentDirectory());
	}
	else if (head == "list" || head == "ls")
	{
		for (const String &entry : current->fileSystem.List())
		{
			Print(entry);
		}
	}
	else if (head == "tree")
	{
		for (const String &entry : current->fileSystem.Tree())
		{
			Print(entry);
		}
	}
	else if (head == "connect")
	{
		Connect(command);
	}
	else if (head == "disconnect")
	{
		Disconnect();
	}
	else if (head == "hosts")
	{
		Hosts();
	}
	else if (head == "passwd")
	{
		Passwd(args);
	}
	else if (head == "hack")
	{
		Hack(args);
	}
	else if (head == "restore")
	{
		Restore();
	}
	else if (head == "get")
	{
		Get(args);
	}
	else if (head == "rekey")
	{
		Rekey();
	}
	else if (head == "who")
	{
		Who();
	}
	else if (head == "ps")
	{
		ListProcesses();
	}
	else if (head == "kill")
	{
		Signal(args, false);
	}
	else if (head == "start")
	{
		Signal(args, true);
	}
	else if (head == "role")
	{
		Tint(args);
	}
	else if (head == "cheat")
	{
		Cheat(command);
	}
	else if (head == "exit")
	{
		requestClose = true;
	}
	else
	{
		Print("Unknown command");
	}
}

void Terminal::Help()
{
	Print("Help Command Descriptions:");
	Print("");
	Print(" - help: Displays a list of available commands and their descriptions");
	Print(" - cd: Changes the current directory to the specified directory");
	Print(" - mkdir: Creates a new directory with the specified name");
	Print(" - mk: Creates a new file with the specified name");
	Print(" - del: Deletes the specified file or directory");
	Print(" - rn: Renames the specified file or directory");
	Print(" - pwd: Prints the current working directory");
	Print(" - ls: Lists the contents of the current directory");
	Print(" - tree: Displays a tree structure of the directories and files starting from the current directory.");
	Print(" - edit: program (Opens a program in the editor)");
	Print(" - run: program (Executes a program. On a computer you are connected to, it runs there and its output comes back)");
	Print(" - ps: Lists the running processes: your buildings and the programs you spawned. On a computer you are connected to, its own");
	Print(" - kill: pid (Stops a running process, here or on a computer you are connected to)");
	Print(" - start: pid (Starts a stopped building again)");
	Print(" - hosts: Lists the other players' computers you can connect to");
	Print(" - who: Shows this computer's pin and who is connected to it");
	Print(" - hack: power (Cuts the grid of the computer you are connected to)");
	Print(" - restore: Puts your own grid back after it has been cut");
	Print(" - get: file (Copies a file from the computer you are connected to into your current directory)");
	Print(" - rekey: Draws a new salt for this computer's radar, so whatever was cracked from it is stale");
	Print(" - passwd: pin (Changes this computer's pin, shutting out anybody connected)");
	Print(" - connect: remote_computer_name pin (Connects to the remote computer)");
	Print(" - disconnect: Disconnects from remote computer");
	Print(" - (connect, hack, get, rekey, passwd and role work only on maps that allow cyber commands)");
	Print(" - role: cyber red | cyber blue | off (Tints the console for the red or blue team, or clears it)");
	Print(" - exit: Exit from Desktop emulates the F10 desktop function");
}

void Terminal::Run(const String &command)
{
	Execute(current->fileSystem, Split(command, ' '));
}

void Terminal::Execute(FileSystem &files, const Vector<String> &args)
{
	if (args.size() < 2)
	{
		Print("Invalid command. Usage: " + (args.empty() ? String("run") : args[0]) + " <fileName>");
		return;
	}

	const String name = args[1];

	if (files.IsDirectory(name))
	{
		Print(name + " is a directory");
		return;
	}

	String source;

	if (!files.Read(name, source))
	{
		Print("File " + name + " doesn't exist");
		return;
	}

	Interpreter interpreter(this);
	interpreter.SetStepLimit(RUN_STEP_LIMIT);

	Vector<String> errors;
	NodeRef program = interpreter.Produce(source, errors);

	if (!errors.empty())
	{
		for (const String &error : errors)
		{
			Print(error);
		}

		return;
	}

	interpreter.Run(program, Vector<String>(args.begin() + 2, args.end()));
}

void Terminal::Edit(const String &command)
{
	const Vector<String> args = Split(command, ' ');

	if (args.size() != 2)
	{
		Print("Invalid command. Usage: " + args[0] + " <fileName>");
		return;
	}

	String source;

	if (!current->fileSystem.Read(args[1], source))
	{
		Print("File " + args[1] + " doesn't exist");
		return;
	}

	editor.Open(args[1], source);
	mode = TerminalMode::Editing;
}

void Terminal::SaveEditor()
{
	if (editTarget)
	{
		const String source = editor.Source();

		if (source.size() > MAX_REMOTE_SOURCE)
		{
			Print(editor.Name() + " is too large to send to " + editTarget->machine + ", the limit is " + std::to_string(MAX_REMOTE_SOURCE / 1024) + " KB");
			return;
		}

		Print("Saving " + editor.Name() + " to " + editTarget->machine + "...");
		Send(editTarget->machine, editTarget->directory, "write", {editor.Name()}, source);

		editTarget.reset();
		editor.ClearDirty();
		editor.Close();

		mode = TerminalMode::Command;
		return;
	}

	if (!editor.Name().empty())
	{
		current->fileSystem.Write(editor.Name(), editor.Source());
		Print("Saved " + editor.Name());
	}

	Persist();

	editor.ClearDirty();
	editor.Close();

	mode = TerminalMode::Command;
}

void Terminal::Connect(const String &command)
{
	const Vector<String> args = Split(command, ' ');

	if (args.size() >= 2 && machines.find(args[1]) != machines.end())
	{
		Session &machine = machines.find(args[1])->second;

		if (args.size() != 3)
		{
			Print("Invalid command. Usage: connect <computer> <pin>");
			return;
		}

		if (current == &machine)
		{
			Print("Already connected to " + args[1]);
			return;
		}

		if (current->networked)
		{
			Disconnect();
		}

		{
			std::lock_guard<std::recursive_mutex> lock(mutex);

			current = &machine;
			current->directory = "/";
		}

		Print("Connecting to " + args[1] + "...");
		Send(current->name, current->directory, "hello", {args[2]});
		return;
	}

	if (args.size() != 4)
	{
		Print("Invalid command. Usage: connect <computer> <pin>");
		return;
	}

	const auto found = remotes.find(args[1]);

	if (found == remotes.end())
	{
		Print("Computer " + args[1] + " not found");
		return;
	}

	if (found->second.user != args[2] || found->second.password != args[3])
	{
		Print("Access denied");
		return;
	}

	if (current->networked)
	{
		Disconnect();
	}

	{
		std::lock_guard<std::recursive_mutex> lock(mutex);
		current = &found->second;
	}

	current->fileSystem.SetCurrentDirectory("/");

	Print("Connected to " + found->second.name);
}

void Terminal::Disconnect()
{
	if (current == &local)
	{
		Print("Not connected to a remote computer");
		return;
	}

	if (current->networked)
	{
		Send(current->name, current->directory, "bye", {});
	}

	Print("Disconnected from " + current->name);

	std::lock_guard<std::recursive_mutex> lock(mutex);
	current = &local;
}

void Terminal::Cheat(const String &command)
{
	const Vector<String> args = Split(command, ' ');

	if (multiplayer)
	{
		Print("Cheats are disabled in multiplayer");
		return;
	}

	if (args.size() == 5 && args[1] == "when" && args[2] == "the" && args[3] == "walls" && args[4] == "fell")
	{
		showFogOfWar = !showFogOfWar;

		Toggle("fogofwar", showFogOfWar ? "show" : "hide", showFogOfWar);

		return;
	}

	Print("Unknown command");
}

bool Terminal::Refuses(const String &head)
{
	static const Set<String> commands = {"connect", "hack", "get", "rekey", "passwd", "role"};

	if (IsCyber() || !commands.contains(head))
	{
		return false;
	}

	Print("Cyber commands are not allowed on this map");
	return true;
}

void Terminal::SetCyber(bool allowed)
{
	{
		std::lock_guard<std::recursive_mutex> lock(mutex);
		cyber = allowed;
	}

	if (!allowed && current->networked)
	{
		Disconnect();
	}
}

bool Terminal::IsCyber() const
{
	std::lock_guard<std::recursive_mutex> lock(mutex);
	return cyber;
}

void Terminal::Tutorial(const String &directory, const nlohmann::json &files)
{
	if (directory.empty() || directory.find('/') != String::npos || !files.is_object())
	{
		return;
	}

	{
		std::lock_guard<std::recursive_mutex> lock(mutex);

		FileNodeRef home = local.fileSystem.Resolve("/home/user/naomi");

		if (!home || !home->directory)
		{
			home = local.fileSystem.GetRoot();
		}

		FileNodeRef &folder = home->children[directory];

		if (!folder)
		{
			folder = std::make_shared<FileNode>();
			folder->directory = true;
		}

		if (!folder->directory)
		{
			return;
		}

		for (const auto &entry : files.items())
		{
			if (!entry.value().is_string() || folder->children.contains(entry.key()))
			{
				continue;
			}

			auto file = std::make_shared<FileNode>();
			file->source = entry.value().get<String>();
			file->executable = true;

			folder->children[entry.key()] = std::move(file);
		}
	}

	Print("A tutorial is waiting in ~/" + directory + ". cd " + directory + ", then ls");
	Persist();
}

void Terminal::Tint(const Vector<String> &args)
{
	String role;

	for (std::size_t index = 1; index < args.size(); ++index)
	{
		role += (index > 1 ? " " : "") + args[index];
	}

	const auto found = Tints().find(role);

	if (found == Tints().end())
	{
		Print("Invalid command. Usage: role cyber <red|blue> | role off");
		return;
	}

	{
		std::lock_guard<std::recursive_mutex> lock(mutex);
		tint = found->second;
	}

	Print(found->second == TerminalTint::None ? "Role cleared" : "Role: " + found->first);
	Persist();
}

void Terminal::Character(char character)
{
	if (mode == TerminalMode::Editing)
	{
		editor.Insert(character);
		return;
	}

	input += character;
}

void Terminal::Backspace()
{
	if (mode == TerminalMode::Editing)
	{
		editor.Backspace();
		return;
	}

	if (!input.empty())
	{
		input.pop_back();
	}
}

void Terminal::Newline()
{
	if (mode == TerminalMode::Editing)
	{
		editor.Newline();
		return;
	}

	const String command = input;
	input.clear();

	Submit(command);
}

void Terminal::Tab()
{
	if (mode == TerminalMode::Editing)
	{
		editor.Tab();
	}
}

void Terminal::Escape()
{
	if (mode == TerminalMode::Editing)
	{
		SaveEditor();
		return;
	}

	requestClose = true;
}

void Terminal::CursorUp()
{
	if (mode == TerminalMode::Editing)
	{
		editor.CursorUp();
		return;
	}

	HistoryUp();
}

void Terminal::CursorDown()
{
	if (mode == TerminalMode::Editing)
	{
		editor.CursorDown();
		return;
	}

	HistoryDown();
}

void Terminal::CursorLeft()
{
	if (mode == TerminalMode::Editing)
	{
		editor.CursorLeft();
	}
}

void Terminal::CursorRight()
{
	if (mode == TerminalMode::Editing)
	{
		editor.CursorRight();
	}
}

void Terminal::HistoryUp()
{
	if (history.empty())
	{
		return;
	}

	if (historyCursor > 0)
	{
		--historyCursor;
	}

	input = history[historyCursor];
}

void Terminal::HistoryDown()
{
	if (history.empty())
	{
		return;
	}

	if (historyCursor + 1 >= history.size())
	{
		historyCursor = history.size();
		input.clear();
		return;
	}

	++historyCursor;
	input = history[historyCursor];
}

Vector<String> Terminal::GetOutput() const
{
	std::lock_guard<std::recursive_mutex> lock(mutex);

	return output;
}

String Terminal::GetPrompt() const
{
	const String &directory = current->networked ? current->directory : current->fileSystem.GetCurrentDirectory();

	return current->user + "@" + current->name + ":" + directory + "> ";
}

String Terminal::GetInput() const
{
	return input;
}

TerminalMode Terminal::GetMode() const
{
	return mode;
}

TerminalTint Terminal::GetTint() const
{
	std::lock_guard<std::recursive_mutex> lock(mutex);
	return tint;
}

Editor &Terminal::GetEditor()
{
	return editor;
}

bool Terminal::ShouldClose()
{
	const bool close = requestClose;
	requestClose = false;

	return close;
}

void Terminal::Hosts()
{
	if (machines.empty())
	{
		Print(multiplayer ? "No other computers on this network" : "Not on a network");
		return;
	}

	Vector<String> names;

	for (const auto &entry : machines)
	{
		names.push_back(entry.first);
	}

	std::ranges::sort(names);

	for (const String &name : names)
	{
		Print(" - " + name);
	}
}

void Terminal::Passwd(const Vector<String> &args)
{
	if (current->networked)
	{
		Print("The pin on " + current->name + " can only be changed from its own console");
		return;
	}

	if (args.size() != 2 || args[1].empty())
	{
		Print("Invalid command. Usage: passwd <pin>");
		return;
	}

	if (args[1].find_first_not_of("0123456789") != String::npos)
	{
		Print("A pin is digits only");
		return;
	}

	{
		std::lock_guard<std::recursive_mutex> lock(mutex);

		local.password = args[1];
		sessions.clear();
	}

	Print("Pin changed. Anybody connected to this computer has been shut out");

	Persist();
}

void Terminal::Who()
{
	std::lock_guard<std::recursive_mutex> lock(mutex);

	Print("This computer's pin is " + local.password);

	if (sessions.empty())
	{
		Print("Nobody is connected to it");
		return;
	}

	for (const String &name : sessions)
	{
		Print(" - " + name + " is connected");
	}
}

void Terminal::SetNetwork(const String &name, const Vector<String> &peers, NetworkSender send)
{
	std::lock_guard<std::recursive_mutex> lock(mutex);

	ClearNetwork();

	self = name;
	sender = std::move(send);
	multiplayer = true;

	Print("This computer's pin is " + local.password + ". Change it with passwd <pin>");

	SetPeers(peers);
}

void Terminal::SetPeers(const Vector<String> &peers)
{
	std::lock_guard<std::recursive_mutex> lock(mutex);

	for (auto entry = machines.begin(); entry != machines.end();)
	{
		if (std::ranges::find(peers, entry->first) != peers.end())
		{
			++entry;
			continue;
		}

		if (current == &entry->second)
		{
			Print(entry->first + " left the network");
			current = &local;
		}

		entry = machines.erase(entry);
	}

	for (const String &peer : peers)
	{
		if (peer.empty() || peer == self || machines.contains(peer))
		{
			continue;
		}

		Session machine;
		machine.name = peer;
		machine.user = local.user;
		machine.networked = true;

		machines[peer] = std::move(machine);
	}
}

void Terminal::ClearNetwork()
{
	std::lock_guard<std::recursive_mutex> lock(mutex);

	if (current->networked)
	{
		current = &local;
	}

	if (editTarget && mode == TerminalMode::Editing)
	{
		editor.Close();
		mode = TerminalMode::Command;
	}

	editTarget.reset();
	machines.clear();
	sessions.clear();
	requests.clear();
	snapshots.clear();
	sightings.clear();
	salt.clear();
	sender = nullptr;
	self.clear();
	multiplayer = false;
}

bool Terminal::Remote(const String &head, const Vector<String> &args)
{
	struct Operation
	{
		String op;
		std::size_t arity;
		String usage;
	};

	static const Map<String, Operation> operations = {
		{"cd", {"cd", 2, " <directory>"}},
		{"changedir", {"cd", 2, " <directory>"}},
		{"mkdir", {"mkdir", 2, " <directory>"}},
		{"makedir", {"mkdir", 2, " <directory>"}},
		{"mk", {"mk", 2, " <fileName>"}},
		{"make", {"mk", 2, " <fileName>"}},
		{"del", {"del", 2, " <fileName>"}},
		{"delete", {"del", 2, " <fileName>"}},
		{"rn", {"rn", 3, " <fileName> <newName>"}},
		{"rename", {"rn", 3, " <fileName> <newName>"}},
		{"edit", {"read", 2, " <fileName>"}},
		{"ed", {"read", 2, " <fileName>"}},
		{"ls", {"ls", 0, ""}},
		{"list", {"ls", 0, ""}},
		{"tree", {"tree", 0, ""}}};

	if (head == "pwd" || head == "print")
	{
		Print(current->directory);
		return true;
	}

	if (head == "run")
	{
		if (args.size() < 2)
		{
			Print("Invalid command. Usage: run <fileName>");
			return true;
		}

		Send(current->name, current->directory, "run", Vector<String>(args.begin() + 1, args.end()));
		return true;
	}

	if (head == "ps")
	{
		Send(current->name, current->directory, "ps", {});
		return true;
	}

	if (head == "restore")
	{
		Print("The grid on " + current->name + " is not yours to restore");
		return true;
	}

	if (head == "rekey")
	{
		Print("The radar on " + current->name + " can only be rekeyed from its own console");
		return true;
	}

	if (head == "kill" || head == "start")
	{
		if (args.size() != 2)
		{
			Print("Invalid command. Usage: " + head + " <pid>");
			return true;
		}

		Send(current->name, current->directory, head, {args[1]});
		return true;
	}

	const auto found = operations.find(head);

	if (found == operations.end())
	{
		return false;
	}

	const Operation &operation = found->second;

	if (operation.arity != 0 && args.size() != operation.arity)
	{
		Print("Invalid command. Usage: " + head + operation.usage);
		return true;
	}

	const Vector<String> operands = operation.arity == 0 ? Vector<String>() : Vector<String>(args.begin() + 1, args.end());

	Send(current->name, current->directory, operation.op, operands);

	return true;
}

void Terminal::Send(const String &machine, const String &directory, const String &op, const Vector<String> &args, const String &source)
{
	if (!sender)
	{
		Print("Not on a network");
		return;
	}

	const int id = ++nextRequest;

	nlohmann::json body = {
		{"kind", "request"},
		{"id", id},
		{"op", op},
		{"cwd", directory},
		{"args", args}};

	if (op == "write")
	{
		body["source"] = source;
	}

	if (op != "bye")
	{
		requests[id] = {machine, op, args.empty() ? String() : args[0], directory, Clock::now()};
	}

	sender(machine, body);
}

void Terminal::Deliver(const nlohmann::json &message)
{
	if (!message.is_object())
	{
		return;
	}

	const String type = message.value("type", String());

	if (type == "shell_refused")
	{
		Refused(message);
		return;
	}

	if (type == "consoles")
	{
		SetPeers(message.value("consoles", Vector<String>{}));
		return;
	}

	if (type != "shell" || !message.contains("body") || !message["body"].is_object())
	{
		return;
	}

	const String from = message.value("from", String());
	const nlohmann::json &body = message["body"];
	const String kind = body.value("kind", String());

	if (from.empty())
	{
		return;
	}

	if (kind == "request")
	{
		Answer(from, body);
	}
	else if (kind == "response")
	{
		Receive(from, body);
	}
}

void Terminal::Answer(const String &from, const nlohmann::json &body)
{
	if (self.empty() || !sender)
	{
		return;
	}

	const String op = body.value("op", String());
	const String directory = body.value("cwd", String("/"));

	Vector<String> args;

	if (body.contains("args") && body["args"].is_array())
	{
		for (const auto &arg : body["args"])
		{
			if (arg.is_string())
			{
				args.push_back(arg.get<String>());
			}
		}
	}

	if (op == "bye")
	{
		std::lock_guard<std::recursive_mutex> lock(mutex);

		sessions.erase(from);

		Print(from + " disconnected from this computer");
		return;
	}

	nlohmann::json reply = {{"kind", "response"}, {"id", body.value("id", 0)}};

	if (!Admit(from, op, op == "hello" && !args.empty() ? args[0] : String(), reply))
	{
		sender(from, reply);
		return;
	}

	Vector<String> lines;
	bool ok = true;
	bool changed = false;

	{
		std::lock_guard<std::recursive_mutex> lock(mutex);

		FileSystem &files = local.fileSystem;
		const FileNodeRef working = files.Resolve(directory);

		if (!working || !working->directory)
		{
			reply["ok"] = false;
			reply["lines"] = {"Directory " + directory + " doesn't exist"};
			reply["cwd"] = "/";

			sender(from, reply);
			return;
		}

		const String saved = files.GetCurrentDirectory();
		files.SetCurrentDirectory(directory);

		const String name = args.empty() ? String() : args[0];
		String message;

		if (op == "hello")
		{
			Print(from + " connected to this computer");
		}
		else if (op == "ls")
		{
			lines = files.List();
		}
		else if (op == "tree")
		{
			lines = files.Tree();
		}
		else if (op == "cd" && args.size() == 1)
		{
			ok = files.ChangeDirectory(name, message);
			lines.push_back(message);
		}
		else if (op == "mkdir" && args.size() == 1)
		{
			ok = files.MakeDirectory(name, message);
			changed = ok;
			lines.push_back(message);
		}
		else if (op == "mk" && args.size() == 1)
		{
			ok = files.MakeFile(name, message);
			changed = ok;
			lines.push_back(message);
		}
		else if (op == "del" && args.size() == 1)
		{
			ok = files.Remove(name, message);
			changed = ok;
			lines.push_back(message);
		}
		else if (op == "rn" && args.size() == 2)
		{
			ok = files.Rename(args[0], args[1], message);
			changed = ok;
			lines.push_back(message);
		}
		else if (op == "read" && args.size() == 1)
		{
			String source;

			if (files.IsDirectory(name))
			{
				ok = false;
				lines.push_back(name + " is a directory");
			}
			else if (!files.Read(name, source))
			{
				ok = false;
				lines.push_back("File " + name + " doesn't exist");
			}
			else
			{
				reply["source"] = source;
			}
		}
		else if (op == "get" && args.size() == 1)
		{
			String source;

			if (files.IsDirectory(name))
			{
				ok = false;
				lines.push_back(name + " is a directory");
			}
			else if (!files.Read(name, source))
			{
				ok = false;
				lines.push_back("File " + name + " doesn't exist");
			}
			else
			{
				const FileNodeRef live = files.Resolve("/sys/radar");
				const auto file = working->children.find(name);

				Print(from + " copied " + name + " from this computer");

				reply["source"] = source;
				reply["radar"] = live && file != working->children.end() && file->second == live;
			}
		}
		else if (op == "run" && !args.empty())
		{
			Print(from + " ran " + args[0] + " on this computer");

			Vector<String> call{"run"};
			call.insert(call.end(), args.begin(), args.end());

			serving = &lines;
			Execute(files, call);
			serving = nullptr;
		}
		else if (op == "ps")
		{
			serving = &lines;
			ListProcesses();
			serving = nullptr;
		}
		else if (op == "hack" && args.size() == 1 && args[0] == "power")
		{
			Print(from + " cut the power on this computer");

			if (!hackRequest)
			{
				ok = false;
				lines.push_back("There is no grid on " + self + " to cut");
			}
			else if (!hackRequest("power", true))
			{
				ok = false;
				lines.push_back("The grid on " + self + " is already cut");
			}
			else
			{
				lines.push_back("The grid on " + self + " is cut");
			}
		}
		else if ((op == "kill" || op == "start") && args.size() == 1)
		{
			Print(from + " sent " + op + " " + args[0] + " to this computer");

			serving = &lines;
			Signal({op, args[0]}, op == "start");
			serving = nullptr;
		}
		else if (op == "write" && args.size() == 1)
		{
			const String source = body.value("source", String());

			if (source.size() > MAX_REMOTE_SOURCE)
			{
				ok = false;
				lines.push_back(name + " is too large, the limit is " + std::to_string(MAX_REMOTE_SOURCE / 1024) + " KB");
			}
			else if (files.IsDirectory(name))
			{
				ok = false;
				lines.push_back(name + " is a directory");
			}
			else
			{
				files.Write(name, source);
				changed = true;
				lines.push_back("Saved " + name);
			}
		}
		else
		{
			ok = false;
			lines.push_back("Unknown request " + op);
		}

		reply["cwd"] = files.GetCurrentDirectory();
		files.SetCurrentDirectory(saved);

		if (changed)
		{
			Persist();
		}
	}

	reply["ok"] = ok;
	reply["lines"] = lines;

	sender(from, reply);
}

bool Terminal::Admit(const String &from, const String &op, const String &pin, nlohmann::json &reply)
{
	std::lock_guard<std::recursive_mutex> lock(mutex);

	const bool known = sessions.find(from) != sessions.end();

	if (op == "hello")
	{
		if (pin != local.password)
		{
			sessions.erase(from);

			Print(from + " was refused a connection to this computer");

			reply["ok"] = false;
			reply["lines"] = {"Access denied"};
			reply["cwd"] = "/";

			return false;
		}

		sessions.insert(from);
		return true;
	}

	if (!known)
	{
		reply["ok"] = false;
		reply["lines"] = {"Access denied"};
		reply["cwd"] = "/";

		return false;
	}

	return true;
}

void Terminal::Receive(const String &from, const nlohmann::json &body)
{
	const auto found = requests.find(body.value("id", 0));

	if (found == requests.end() || found->second.machine != from)
	{
		return;
	}

	const Request request = found->second;
	requests.erase(found);

	const bool ok = body.value("ok", false);

	const auto machine = machines.find(from);

	if (machine != machines.end() && body.contains("cwd") && body["cwd"].is_string())
	{
		machine->second.directory = body["cwd"].get<String>();
	}

	if (request.op == "hello")
	{
		if (ok)
		{
			Print("Connected to " + from);
		}
		else
		{
			Abandon(request, "Computer " + from + " denied access");
		}

		return;
	}

	if (request.op == "get" && ok)
	{
		const String source = body.value("source", String());

		{
			std::lock_guard<std::recursive_mutex> lock(mutex);

			if (local.fileSystem.IsDirectory(request.name))
			{
				Print(request.name + " is a directory here, so " + from + "'s copy was not saved");
				return;
			}

			local.fileSystem.Write(request.name, source);

			if (body.value("radar", false))
			{
				Keep(from, source);
			}
		}

		Print("Copied " + request.name + " from " + from);

		Persist();
		return;
	}

	if (request.op == "read" && ok)
	{
		if (mode == TerminalMode::Editing)
		{
			Print("Close the editor before opening " + request.name);
			return;
		}

		editor.Open(request.name, body.value("source", String()));
		editTarget = EditTarget{from, request.directory};
		mode = TerminalMode::Editing;
		return;
	}

	if (body.contains("lines") && body["lines"].is_array())
	{
		for (const auto &line : body["lines"])
		{
			if (line.is_string())
			{
				Print(line.get<String>());
			}
		}
	}
}

void Terminal::Refused(const nlohmann::json &message)
{
	const auto found = requests.find(message.value("id", 0));

	if (found == requests.end())
	{
		return;
	}

	const Request request = found->second;
	requests.erase(found);

	Abandon(request, "Computer " + request.machine + " " + message.value("reason", String("refused the request")));
}

void Terminal::Abandon(const Request &request, const String &reason)
{
	Print(reason);

	if (request.op == "write")
	{
		Print("Changes to " + request.name + " were not saved");
	}

	if (request.op == "hello" && current->networked && current->name == request.machine)
	{
		std::lock_guard<std::recursive_mutex> lock(mutex);
		current = &local;
	}
}

void Terminal::Update()
{
	const Clock::time_point now = Clock::now();

	for (auto entry = requests.begin(); entry != requests.end();)
	{
		if (now - entry->second.sent < REQUEST_TIMEOUT)
		{
			++entry;
			continue;
		}

		const Request request = entry->second;
		entry = requests.erase(entry);

		Abandon(request, "No answer from " + request.machine);
	}

	if (processLister && now - refreshed >= PROCESS_REFRESH)
	{
		RefreshProcesses();
	}

	if (radarLister && now - published >= radar.publish)
	{
		PublishRadar();
	}

	Vector<Sighting> seen;
	RadarReveal reveal;

	{
		std::lock_guard<std::recursive_mutex> lock(mutex);

		seen.swap(sightings);
		reveal = radarReveal;
	}

	if (reveal)
	{
		for (const Sighting &sighting : seen)
		{
			reveal(sighting.x, sighting.y, sighting.cell);
		}
	}
}

void Terminal::SetProcessHandlers(ProcessLister lister, ProcessSwitch switcher)
{
	std::lock_guard<std::recursive_mutex> lock(mutex);

	processLister = std::move(lister);
	processSwitch = std::move(switcher);
	processes = nlohmann::json::object();
	buildingPids.clear();
}

void Terminal::SetHackHandler(HackRequest handler)
{
	std::lock_guard<std::recursive_mutex> lock(mutex);

	hackRequest = std::move(handler);
}

void Terminal::Hack(const Vector<String> &args)
{
	if (!current->networked)
	{
		Print("There is nothing to hack from your own console. Connect to somebody first");
		return;
	}

	if (args.size() != 2 || args[1] != "power")
	{
		Print("Invalid command. Usage: hack power");
		return;
	}

	Send(current->name, current->directory, "hack", {args[1]});
}

void Terminal::Restore()
{
	if (current->networked)
	{
		Print("The grid on " + current->name + " is not yours to restore");
		return;
	}

	if (!hackRequest)
	{
		Print("There is no grid here to restore");
		return;
	}

	if (!hackRequest("power", false))
	{
		Print("Your grid has not been cut");
		return;
	}

	Print("Restoring the grid");
}

void Terminal::SetRadarHandlers(RadarLister lister, RadarReveal reveal)
{
	std::lock_guard<std::recursive_mutex> lock(mutex);

	radarLister = std::move(lister);
	radarReveal = std::move(reveal);
	salt.clear();
	sightings.clear();
}

void Terminal::SetRadar(const RadarSettings &settings)
{
	std::lock_guard<std::recursive_mutex> lock(mutex);

	radar = settings;
	radar.saltDigits = std::clamp(radar.saltDigits, 1, 9);
	radar.cell = std::max(1, radar.cell);
	salt.clear();
}

void Terminal::PublishRadar()
{
	published = Clock::now();

	if (!radarLister)
	{
		return;
	}

	const nlohmann::json listing = radarLister();

	if (!listing.is_object())
	{
		return;
	}

	std::lock_guard<std::recursive_mutex> lock(mutex);

	if (salt.empty() || (!listing.value("cut", false) && published - rotated >= radar.rotate))
	{
		salt = Digits(radar.saltDigits);
		rotated = published;
	}

	Set<String> hashes;

	if (listing.contains("cells") && listing["cells"].is_array())
	{
		for (const auto &cell : listing["cells"])
		{
			if (cell.is_array() && cell.size() == 2 && cell[0].is_number_integer() && cell[1].is_number_integer())
			{
				hashes.insert(Digest({salt, std::to_string(cell[0].get<int>() / radar.cell), std::to_string(cell[1].get<int>() / radar.cell)}));
			}
		}
	}

	String text = "map " + std::to_string(listing.value("width", 0)) + " " + std::to_string(listing.value("height", 0)) + " " + std::to_string(radar.cell) + "\n";
	text += "check " + Digest({salt}) + "\n";

	for (const String &hash : hashes)
	{
		text += hash + "\n";
	}

	FileSystem &files = local.fileSystem;
	const String saved = files.GetCurrentDirectory();
	String message;

	files.SetCurrentDirectory("/");

	if (!files.IsDirectory("sys"))
	{
		files.MakeDirectory("sys", message);
	}

	files.SetCurrentDirectory("/sys");
	files.Write("radar", text);
	files.SetCurrentDirectory(saved);
}

void Terminal::Rekey()
{
	if (current->networked)
	{
		Print("The radar on " + current->name + " can only be rekeyed from its own console");
		return;
	}

	if (!radarLister)
	{
		Print("There is no radar on this computer to rekey");
		return;
	}

	const nlohmann::json listing = radarLister();

	if (listing.is_object() && listing.value("cut", false))
	{
		Print("The grid is cut, so the radar cannot be rekeyed until it is restored");
		return;
	}

	{
		std::lock_guard<std::recursive_mutex> lock(mutex);
		salt.clear();
	}

	PublishRadar();

	Print("Radar rekeyed. Whatever was cracked from it is stale");
}

void Terminal::Get(const Vector<String> &args)
{
	if (!current->networked)
	{
		Print("get copies a file from a computer you are connected to. Connect to somebody first");
		return;
	}

	if (args.size() != 2)
	{
		Print("Invalid command. Usage: get <fileName>");
		return;
	}

	Send(current->name, current->directory, "get", {args[1]});
}

void Terminal::Keep(const String &machine, const String &source)
{
	Snapshot snapshot;

	for (const String &line : Split(source, '\n'))
	{
		const Vector<String> words = Split(Trim(line), ' ');

		if (words.size() == 4 && words[0] == "map")
		{
			try
			{
				snapshot.cell = std::max(1, std::stoi(words[3]));
			}
			catch (const std::exception &)
			{
				snapshot.cell = 1;
			}
		}
		else if (words.size() == 2 && words[0] == "check")
		{
			snapshot.check = words[1];
		}
		else if (words.size() == 1)
		{
			snapshot.hashes.insert(words[0]);
		}
	}

	if (!snapshot.check.empty())
	{
		snapshots[machine] = std::move(snapshot);
	}
}

bool Terminal::Reveal(const String &key, int x, int y)
{
	std::lock_guard<std::recursive_mutex> lock(mutex);

	if (serving != nullptr)
	{
		Print("reveal is refused from a remote session");
		return false;
	}

	if (!radarReveal)
	{
		return false;
	}

	const String check = Digest({key});
	const String hash = Digest({key, std::to_string(x), std::to_string(y)});

	for (const auto &[machine, snapshot] : snapshots)
	{
		if (snapshot.check == check && snapshot.hashes.find(hash) != snapshot.hashes.end())
		{
			sightings.push_back({x, y, snapshot.cell});
			return true;
		}
	}

	return false;
}

void Terminal::RefreshProcesses()
{
	refreshed = Clock::now();

	if (!processLister)
	{
		return;
	}

	const nlohmann::json listing = processLister();

	std::lock_guard<std::recursive_mutex> lock(mutex);

	processes = listing.is_object() ? listing : nlohmann::json::object();

	std::map<int, int> seen;

	if (processes.contains("processes") && processes["processes"].is_array())
	{
		for (const auto &entry : processes["processes"])
		{
			const int uid = entry.value("uid", -1);
			const auto found = buildingPids.find(uid);

			seen[uid] = found != buildingPids.end() ? found->second : nextPid++;
		}
	}

	buildingPids.swap(seen);
}

void Terminal::ListProcesses()
{
	struct Row
	{
		int pid;
		String state;
		String power;
		String name;
	};

	RefreshProcesses();

	Vector<Row> rows;

	{
		std::lock_guard<std::recursive_mutex> lock(mutex);

		if (processes.contains("processes") && processes["processes"].is_array())
		{
			for (const auto &entry : processes["processes"])
			{
				const auto found = buildingPids.find(entry.value("uid", -1));

				if (found == buildingPids.end())
				{
					continue;
				}

				const bool running = entry.value("running", false);
				const int power = entry.value("power", 0);

				String draw = "0";

				if (running && power < 0)
				{
					draw = "+" + std::to_string(-power);
				}
				else if (running && power > 0)
				{
					draw = "-" + std::to_string(power);
				}

				rows.push_back({found->second, running ? "running" : "stopped", draw, entry.value("name", String())});
			}
		}
	}

	for (const ProcessInfo &process : pool.List())
	{
		rows.push_back({process.pid, process.running ? "running" : "queued", "-", process.name + " (program)"});
	}

	if (rows.empty())
	{
		Print("No processes running");
		return;
	}

	std::ranges::sort(rows, [](const Row &a, const Row &b) { return a.pid < b.pid; });

	const auto pad = [](const String &text, std::size_t width) {
		return text.size() >= width ? text : String(width - text.size(), ' ') + text;
	};

	Print(pad("PID", 5) + "  STATE    " + pad("POWER", 6) + "  NAME");

	for (const Row &row : rows)
	{
		Print(pad(std::to_string(row.pid), 5) + "  " + row.state + String(9 - row.state.size(), ' ') + pad(row.power, 6) + "  " + row.name);
	}

	if (processes.contains("usage") && processes.contains("total"))
	{
		Print("Power " + std::to_string(processes.value("usage", 0)) + " / " + std::to_string(processes.value("total", 0)) +
			  (processes.value("cut", false) ? " (cut)" : ""));
	}
}

void Terminal::Signal(const Vector<String> &args, bool start)
{
	if (args.size() != 2)
	{
		Print("Invalid command. Usage: " + args[0] + " <pid>");
		return;
	}

	int pid = 0;

	try
	{
		std::size_t used = 0;
		pid = std::stoi(args[1], &used);

		if (used != args[1].size())
		{
			pid = 0;
		}
	}
	catch (const std::exception &)
	{
		pid = 0;
	}

	if (pid <= 0)
	{
		Print("Invalid pid " + args[1]);
		return;
	}

	if (!start && pool.Kill(pid))
	{
		Print("Killed " + std::to_string(pid));
		return;
	}

	RefreshProcesses();

	int uid = -1;

	for (const auto &[building, assigned] : buildingPids)
	{
		if (assigned == pid)
		{
			uid = building;
			break;
		}
	}

	if (uid < 0)
	{
		const bool program = start && std::ranges::any_of(pool.List(), [pid](const ProcessInfo &process) { return process.pid == pid; });

		Print(program ? "Process " + std::to_string(pid) + " is already running" : "No such process " + std::to_string(pid));
		return;
	}

	String name;
	bool running = false;

	for (const auto &entry : processes["processes"])
	{
		if (entry.value("uid", -1) == uid)
		{
			name = entry.value("name", String());
			running = entry.value("running", false);
		}
	}

	if (running == start)
	{
		Print("Process " + std::to_string(pid) + (start ? " is already running" : " is already stopped"));
		return;
	}

	if (!processSwitch || !processSwitch(uid, start))
	{
		Print("Process " + std::to_string(pid) + (start ? " could not be started" : " could not be stopped"));
		return;
	}

	Print((start ? "Starting " : "Stopping ") + name + " (" + std::to_string(pid) + ")");
}

void Terminal::Seed()
{
	String message;

	local.fileSystem.Reset();
	local.fileSystem.MakeDirectory("home", message);
	local.fileSystem.ChangeDirectory("home", message);
	local.fileSystem.MakeDirectory("user", message);
	local.fileSystem.ChangeDirectory("user", message);
	local.fileSystem.MakeDirectory("naomi", message);
	local.fileSystem.ChangeDirectory("naomi", message);

	Session remote;
	remote.name = "sc-47-012";
	remote.user = "jamie.turner";
	remote.password = "3487";

	remotes["sc_47_012"] = std::move(remote);

	current = &local;
}

void Terminal::Load(const String &path)
{
	savePath = path;

	std::ifstream stream(path);

	if (!stream.is_open())
	{
		Seed();
		return;
	}

	nlohmann::json data;

	try
	{
		stream >> data;
	}
	catch (const std::exception &)
	{
		Seed();
		return;
	}

	Seed();

	if (data.contains("local"))
	{
		local.fileSystem.Deserialise(data["local"]);
	}

	if (data.contains("pin") && data["pin"].is_string())
	{
		local.password = data["pin"].get<String>();
	}

	if (data.contains("role") && data["role"].is_string())
	{
		const auto found = Tints().find(data["role"].get<String>());
		tint = found == Tints().end() ? TerminalTint::None : found->second;
	}

	if (data.contains("remotes") && data["remotes"].is_object())
	{
		for (const auto &entry : data["remotes"].items())
		{
			const auto found = remotes.find(entry.key());

			if (found != remotes.end())
			{
				found->second.fileSystem.Deserialise(entry.value());
			}
		}
	}

	current = &local;
}

void Terminal::Persist()
{
	if (!savePath.empty())
	{
		Save(savePath);
	}
}

void Terminal::CommitEditor()
{
	if (mode == TerminalMode::Editing)
	{
		SaveEditor();
	}
}

void Terminal::Save(const String &path) const
{
	std::lock_guard<std::recursive_mutex> lock(mutex);

	nlohmann::json data;
	data["local"] = local.fileSystem.Serialise();
	data["pin"] = local.password;
	data["role"] = TintName(tint);
	data["remotes"] = nlohmann::json::object();

	for (const auto &entry : remotes)
	{
		data["remotes"][entry.first] = entry.second.fileSystem.Serialise();
	}

	std::ofstream stream(path);

	if (!stream.is_open())
	{
		return;
	}

	stream << data.dump(1, '\t');
}
} // namespace TGX::Shell
