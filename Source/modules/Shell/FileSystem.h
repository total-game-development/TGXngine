#pragma once

#include <map>
#include <nlohmann/json.hpp>
#include "Core.h"

namespace TGX::Shell
{
struct FileNode;
using FileNodeRef = Ref<FileNode>;

struct FileNode
{
	bool directory = false;
	bool executable = false;
	String source;
	std::map<String, FileNodeRef> children;
};

class FileSystem
{
private:
	FileNodeRef root;
	String currentDirectory = "/";

	static Vector<String> Tokenise(const String &path);

public:
	FileSystem();

	void Reset();

	FileNodeRef GetRoot() const;
	FileNodeRef GetWorkingDirectory() const;
	FileNodeRef Resolve(const String &path) const;

	const String &GetCurrentDirectory() const;
	void SetCurrentDirectory(const String &path);

	bool ChangeDirectory(const String &name, String &message);
	bool MakeDirectory(const String &name, String &message);
	bool MakeFile(const String &name, String &message);
	bool Remove(const String &name, String &message);
	bool Rename(const String &from, const String &to, String &message);

	Vector<String> List() const;
	Vector<String> Tree() const;

	bool Read(const String &name, String &source) const;
	void Write(const String &name, const String &source);
	bool IsDirectory(const String &name) const;
	bool Exists(const String &name) const;

	nlohmann::json Serialise() const;
	void Deserialise(const nlohmann::json &data);
};
} // namespace TGX::Shell
