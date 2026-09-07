#include "Terminal.h"
#include <fstream>
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
} // namespace

Terminal::Terminal()
{
	local.name = "local";
	local.user = "naomi";
	current = &local;
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

	output.push_back(message);

	while (output.size() > MAX_OUTPUT)
	{
		output.erase(output.begin());
	}
}

bool Terminal::ReadFile(const String &name, String &source)
{
	std::lock_guard<std::recursive_mutex> lock(mutex);

	return current->fileSystem.Read(name, source);
}

void Terminal::WriteFile(const String &name, const String &source)
{
	std::lock_guard<std::recursive_mutex> lock(mutex);

	current->fileSystem.Write(name, source);
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

		if (!current->fileSystem.Read(args[0], source))
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
	task.program = program;
	task.args = Vector<String>(args.begin() + 1, args.end());

	pool.Enqueue(std::move(task));
}

void Terminal::Submit(const String &line)
{
	const String command = Trim(line);

	Print(GetPrompt() + command);

	if (!command.empty())
	{
		history.push_back(command);

		while (history.size() > MAX_HISTORY)
		{
			history.erase(history.begin());
		}
	}

	historyCursor = history.size();

	if (command.empty())
	{
		return;
	}

	if (command.rfind("./", 0) == 0)
	{
		Run("run " + command.substr(2));
		return;
	}

	const Vector<String> args = Split(command, ' ');
	const String head = args.empty() ? String() : args[0];

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
	Print(" - run: program (Executes a program)");
	Print(" - connect: remote_computer_name firstname.lastname pin (Connects to the remote computer)");
	Print(" - disconnect: Disconnects from remote computer");
	Print(" - exit: Exit from Desktop emulates the F10 desktop function");
}

void Terminal::Run(const String &command)
{
	const Vector<String> args = Split(command, ' ');

	if (args.size() < 2)
	{
		Print("Invalid command. Usage: " + (args.empty() ? String("run") : args[0]) + " <fileName>");
		return;
	}

	const String name = args[1];

	if (current->fileSystem.IsDirectory(name))
	{
		Print(name + " is a directory");
		return;
	}

	String source;

	if (!current->fileSystem.Read(name, source))
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
	if (!editor.Name().empty())
	{
		current->fileSystem.Write(editor.Name(), editor.Source());
		Print("Saved " + editor.Name());
	}

	editor.ClearDirty();
	editor.Close();

	mode = TerminalMode::Command;
}

void Terminal::Connect(const String &command)
{
	const Vector<String> args = Split(command, ' ');

	if (args.size() != 4)
	{
		Print("Invalid command. Usage: connect <computer> <firstname.lastname> <pin>");
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

	current = &found->second;
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

	Print("Disconnected from " + current->name);

	current = &local;
}

void Terminal::Cheat(const String &command)
{
	const Vector<String> args = Split(command, ' ');

	if (args.size() != 2)
	{
		Print("Invalid command. Usage: cheat <code>");
		return;
	}

	Print("Cheat " + args[1] + " accepted");
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
	}
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
	return current->user + "@" + current->name + ":" + current->fileSystem.GetCurrentDirectory() + "> ";
}

String Terminal::GetInput() const
{
	return input;
}

TerminalMode Terminal::GetMode() const
{
	return mode;
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

void Terminal::Save(const String &path) const
{
	nlohmann::json data;
	data["local"] = local.fileSystem.Serialise();
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
