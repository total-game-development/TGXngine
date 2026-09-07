#pragma once

#include "Ast.h"
#include "Environment.h"
#include "Host.h"
#include "Parser.h"
#include "Value.h"

namespace TGX::Shell
{
class Interpreter
{
private:
	Host *host = nullptr;
	EnvironmentArena arena;
	Parser parser;

	bool breakFlag = false;
	bool continueFlag = false;
	bool aborted = false;
	int depth = 0;

	unsigned long long steps = 0;
	unsigned long long stepLimit = 0;

	void Fail(const String &message);

	ValueRef InterpretProgram(const NodeRef &node, Environment *environment);
	ValueRef InterpretVariableDeclaration(const NodeRef &node, Environment *environment);
	ValueRef InterpretFunctionDeclaration(const NodeRef &node, Environment *environment);
	ValueRef InterpretPrint(const NodeRef &node, Environment *environment);
	ValueRef InterpretToggle(const NodeRef &node, Environment *environment);
	ValueRef InterpretSplit(const NodeRef &node, Environment *environment);
	ValueRef InterpretLength(const NodeRef &node, Environment *environment);
	ValueRef InterpretRead(const NodeRef &node, Environment *environment);
	ValueRef InterpretWrite(const NodeRef &node, Environment *environment);
	ValueRef InterpretReturn(const NodeRef &node, Environment *environment);
	ValueRef InterpretExec(const NodeRef &node, Environment *environment);
	ValueRef InterpretSpawn(const NodeRef &node, Environment *environment);
	ValueRef InterpretIf(const NodeRef &node, Environment *environment);
	ValueRef InterpretWhile(const NodeRef &node, Environment *environment);
	ValueRef InterpretFor(const NodeRef &node, Environment *environment);
	ValueRef InterpretIdentifier(const NodeRef &node, Environment *environment);
	ValueRef InterpretObject(const NodeRef &node, Environment *environment);
	ValueRef InterpretArray(const NodeRef &node, Environment *environment);
	ValueRef InterpretMember(const NodeRef &node, Environment *environment);
	ValueRef InterpretAssignment(const NodeRef &node, Environment *environment);
	ValueRef InterpretBinary(const NodeRef &node, Environment *environment);
	ValueRef InterpretUnary(const NodeRef &node, Environment *environment);
	ValueRef InterpretLogical(const NodeRef &node, Environment *environment);
	ValueRef InterpretCall(const NodeRef &node, Environment *environment);

	bool RunBody(const Vector<NodeRef> &body, Environment *environment, ValueRef &result);

public:
	explicit Interpreter(Host *inHost);

	void SetStepLimit(unsigned long long limit);

	ValueRef Interpret(const NodeRef &node, Environment *environment);
	void Run(const NodeRef &program, const Vector<String> &args);
	NodeRef Produce(const String &source, Vector<String> &errors);
};
} // namespace TGX::Shell
