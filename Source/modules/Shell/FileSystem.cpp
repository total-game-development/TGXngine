#include "FileSystem.h"

namespace TGX::Shell
{
namespace
{
FileNodeRef MakeDirectoryNode()
{
	FileNodeRef node = std::make_shared<FileNode>();
	node->directory = true;
	return node;
}

FileNodeRef MakeFileNode(const String &source, bool executable)
{
	FileNodeRef node = std::make_shared<FileNode>();
	node->directory = false;
	node->source = source;
	node->executable = executable;
	return node;
}

void CollectTree(const FileNodeRef &node, const String &name, int indent, Vector<String> &lines)
{
	if (!name.empty())
	{
		lines.push_back(String(static_cast<std::size_t>(indent), ' ') + name + (node->directory ? "/" : ""));
	}

	if (!node->directory)
	{
		return;
	}

	for (const auto &child : node->children)
	{
		CollectTree(child.second, child.first, name.empty() ? 0 : indent + 4, lines);
	}
}
} // namespace

FileSystem::FileSystem()
{
	Reset();
}

void FileSystem::Reset()
{
	root = MakeDirectoryNode();
	currentDirectory = "/";
}

Vector<String> FileSystem::Tokenise(const String &path)
{
	Vector<String> parts;
	String current;

	for (const char character : path)
	{
		if (character == '/')
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

FileNodeRef FileSystem::GetRoot() const
{
	return root;
}

FileNodeRef FileSystem::Resolve(const String &path) const
{
	FileNodeRef node = root;

	for (const String &part : Tokenise(path))
	{
		if (!node || !node->directory)
		{
			return nullptr;
		}

		const auto found = node->children.find(part);

		if (found == node->children.end())
		{
			return nullptr;
		}

		node = found->second;
	}

	return node;
}

FileNodeRef FileSystem::GetWorkingDirectory() const
{
	return Resolve(currentDirectory);
}

const String &FileSystem::GetCurrentDirectory() const
{
	return currentDirectory;
}

void FileSystem::SetCurrentDirectory(const String &path)
{
	currentDirectory = path;
}

bool FileSystem::ChangeDirectory(const String &name, String &message)
{
	if (name == "/")
	{
		currentDirectory = "/";
		message = "Changing to root directory";
		return true;
	}

	if (name == ".." || name == "../")
	{
		if (currentDirectory == "/")
		{
			message = "Already at root directory";
			return false;
		}

		Vector<String> parts = Tokenise(currentDirectory);
		parts.pop_back();

		currentDirectory = "/";

		for (const String &part : parts)
		{
			currentDirectory += part + "/";
		}

		message = "Changing to directory to " + currentDirectory;
		return true;
	}

	FileNodeRef working = GetWorkingDirectory();

	if (!working)
	{
		message = "Directory " + name + " doesn't exist";
		return false;
	}

	const auto found = working->children.find(name);

	if (found == working->children.end())
	{
		message = "Directory " + name + " doesn't exist";
		return false;
	}

	if (!found->second->directory)
	{
		message = name + " is a file";
		return false;
	}

	currentDirectory += name + "/";
	message = "Changing to directory to " + currentDirectory;

	return true;
}

bool FileSystem::MakeDirectory(const String &name, String &message)
{
	FileNodeRef working = GetWorkingDirectory();

	if (!working)
	{
		message = "Working directory is invalid";
		return false;
	}

	if (working->children.find(name) != working->children.end())
	{
		message = working->children[name]->directory ? "Directory already exists" : "File already exists";
		return false;
	}

	working->children[name] = MakeDirectoryNode();
	message = "Directory " + name + " created";

	return true;
}

bool FileSystem::MakeFile(const String &name, String &message)
{
	FileNodeRef working = GetWorkingDirectory();

	if (!working)
	{
		message = "Working directory is invalid";
		return false;
	}

	if (working->children.find(name) != working->children.end())
	{
		message = working->children[name]->directory ? "Directory already exists" : "File already exists";
		return false;
	}

	working->children[name] = MakeFileNode("", true);
	message = "File " + name + " created";

	return true;
}

bool FileSystem::Remove(const String &name, String &message)
{
	FileNodeRef working = GetWorkingDirectory();

	if (!working)
	{
		message = "Working directory is invalid";
		return false;
	}

	const auto found = working->children.find(name);

	if (found == working->children.end())
	{
		message = name + " doesn't exist";
		return false;
	}

	working->children.erase(found);
	message = name + " deleted";

	return true;
}

bool FileSystem::Rename(const String &from, const String &to, String &message)
{
	FileNodeRef working = GetWorkingDirectory();

	if (!working)
	{
		message = "Working directory is invalid";
		return false;
	}

	const auto found = working->children.find(from);

	if (found == working->children.end())
	{
		message = from + " doesn't exist";
		return false;
	}

	if (working->children.find(to) != working->children.end())
	{
		message = to + " already exists";
		return false;
	}

	FileNodeRef node = found->second;
	working->children.erase(found);
	working->children[to] = node;

	message = from + " renamed to " + to;

	return true;
}

Vector<String> FileSystem::List() const
{
	Vector<String> entries;

	FileNodeRef working = GetWorkingDirectory();

	if (!working)
	{
		return entries;
	}

	for (const auto &child : working->children)
	{
		entries.push_back(child.first + (child.second->directory ? "/" : ""));
	}

	return entries;
}

Vector<String> FileSystem::Tree() const
{
	Vector<String> lines;

	FileNodeRef working = GetWorkingDirectory();

	if (working)
	{
		CollectTree(working, "", 0, lines);
	}

	return lines;
}

bool FileSystem::Exists(const String &name) const
{
	FileNodeRef working = GetWorkingDirectory();

	return working && working->children.find(name) != working->children.end();
}

bool FileSystem::IsDirectory(const String &name) const
{
	FileNodeRef working = GetWorkingDirectory();

	if (!working)
	{
		return false;
	}

	const auto found = working->children.find(name);

	return found != working->children.end() && found->second->directory;
}

bool FileSystem::Read(const String &name, String &source) const
{
	FileNodeRef working = GetWorkingDirectory();

	if (!working)
	{
		return false;
	}

	const auto found = working->children.find(name);

	if (found == working->children.end() || found->second->directory)
	{
		return false;
	}

	source = found->second->source;

	return true;
}

void FileSystem::Write(const String &name, const String &source)
{
	FileNodeRef working = GetWorkingDirectory();

	if (!working)
	{
		return;
	}

	const auto found = working->children.find(name);

	if (found != working->children.end() && !found->second->directory)
	{
		found->second->source = source;
		found->second->executable = true;
		return;
	}

	if (found != working->children.end())
	{
		return;
	}

	working->children[name] = MakeFileNode(source, true);
}

nlohmann::json FileSystem::Serialise() const
{
	Function<nlohmann::json(const FileNodeRef &)> encode = [&encode](const FileNodeRef &node) {
		nlohmann::json entry;

		if (node->directory)
		{
			entry["directory"] = true;
			entry["children"] = nlohmann::json::object();

			for (const auto &child : node->children)
			{
				entry["children"][child.first] = encode(child.second);
			}

			return entry;
		}

		entry["directory"] = false;
		entry["source"] = node->source;
		entry["executable"] = node->executable;

		return entry;
	};

	nlohmann::json data;
	data["currentDirectory"] = currentDirectory;
	data["root"] = encode(root);

	return data;
}

void FileSystem::Deserialise(const nlohmann::json &data)
{
	Reset();

	if (!data.is_object() || !data.contains("root"))
	{
		return;
	}

	Function<FileNodeRef(const nlohmann::json &)> decode = [&decode](const nlohmann::json &entry) -> FileNodeRef {
		if (!entry.is_object())
		{
			return MakeFileNode("", true);
		}

		const bool directory = entry.value("directory", false);

		if (!directory)
		{
			return MakeFileNode(entry.value("source", String()), entry.value("executable", true));
		}

		FileNodeRef node = MakeDirectoryNode();

		if (entry.contains("children") && entry["children"].is_object())
		{
			for (const auto &child : entry["children"].items())
			{
				node->children[child.key()] = decode(child.value());
			}
		}

		return node;
	};

	root = decode(data["root"]);
	currentDirectory = data.value("currentDirectory", String("/"));

	if (!Resolve(currentDirectory))
	{
		currentDirectory = "/";
	}
}
} // namespace TGX::Shell
