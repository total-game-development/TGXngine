#include "Environment.h"

namespace TGX::Shell
{
Environment::Environment(Environment *inParent)
	: parent(inParent)
{
}

void Environment::DeclareDefaults()
{
	String error;

	DeclareVariable("true", MakeBoolean(true), true, error);
	DeclareVariable("false", MakeBoolean(false), true, error);
	DeclareVariable("args", MakeArray({}), true, error);
	DeclareVariable("out", MakeNull(), false, error);
}

Environment *Environment::Resolve(const String &name, bool &found)
{
	if (variables.find(name) != variables.end())
	{
		found = true;
		return this;
	}

	if (parent == nullptr)
	{
		found = false;
		return this;
	}

	return parent->Resolve(name, found);
}

bool Environment::Has(const String &name)
{
	bool found = false;
	Resolve(name, found);
	return found;
}

ValueRef Environment::DeclareVariable(const String &name, ValueRef value, bool constant, String &error)
{
	if (variables.find(name) != variables.end())
	{
		error = "Cannot declare variable " + name + ". As it already is defined.";
		return MakeNull();
	}

	variables[name] = value;

	if (constant)
	{
		constants.insert(name);
	}

	return value;
}

ValueRef Environment::AssignVariable(const String &name, ValueRef value, String &error)
{
	bool found = false;
	Environment *environment = Resolve(name, found);

	if (!found)
	{
		error = "Cannot resolve " + name + " as it does not exist.";
		return MakeNull();
	}

	if (environment->constants.find(name) != environment->constants.end())
	{
		error = "Cannot reassign to variable " + name + ". As it declared constant.";
		return MakeNull();
	}

	environment->variables[name] = value;

	return value;
}

ValueRef Environment::LookupVariable(const String &name, String &error)
{
	bool found = false;
	Environment *environment = Resolve(name, found);

	if (!found)
	{
		error = "Cannot resolve " + name + " as it does not exist.";
		return MakeNull();
	}

	return environment->variables[name];
}

Environment *EnvironmentArena::Create(Environment *parent)
{
	environments.push_back(std::make_unique<Environment>(parent));
	return environments.back().get();
}

void EnvironmentArena::Clear()
{
	environments.clear();
}
} // namespace TGX::Shell
