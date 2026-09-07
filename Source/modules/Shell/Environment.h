#pragma once

#include "Core.h"
#include "Value.h"

namespace TGX::Shell
{
class Environment
{
private:
	Environment *parent = nullptr;
	Map<String, ValueRef> variables;
	Set<String> constants;

	Environment *Resolve(const String &name, bool &found);

public:
	explicit Environment(Environment *inParent = nullptr);

	void DeclareDefaults();

	ValueRef DeclareVariable(const String &name, ValueRef value, bool constant, String &error);
	ValueRef AssignVariable(const String &name, ValueRef value, String &error);
	ValueRef LookupVariable(const String &name, String &error);
	bool Has(const String &name);
};

class EnvironmentArena
{
private:
	Vector<Unique<Environment>> environments;

public:
	Environment *Create(Environment *parent);
	void Clear();
};
} // namespace TGX::Shell
