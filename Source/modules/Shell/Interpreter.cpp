#include "Interpreter.h"
#include <cmath>
#include <cstdlib>

namespace TGX::Shell
{
namespace
{
constexpr int MAX_DEPTH = 256;

double NumberOf(const ValueRef &value)
{
	if (!value)
	{
		return 0.0;
	}

	switch (value->type)
	{
		case ValueType::Number:
			return value->number;
		case ValueType::Boolean:
			return value->boolean ? 1.0 : 0.0;
		case ValueType::String:
			return std::strtod(value->string.c_str(), nullptr);
		default:
			return 0.0;
	}
}

bool BothStrings(const ValueRef &left, const ValueRef &right)
{
	return left && right && (left->type == ValueType::String || right->type == ValueType::String);
}

bool Equals(const ValueRef &left, const ValueRef &right)
{
	if (!left || !right)
	{
		return left == right;
	}

	if (BothStrings(left, right))
	{
		return Stringify(left) == Stringify(right);
	}

	return NumberOf(left) == NumberOf(right);
}
} // namespace

Interpreter::Interpreter(Host *inHost)
	: host(inHost)
{
}

void Interpreter::SetStepLimit(unsigned long long limit)
{
	stepLimit = limit;
}

void Interpreter::Fail(const String &message)
{
	if (aborted)
	{
		return;
	}

	aborted = true;

	if (host != nullptr)
	{
		host->Print(message);
	}
}

NodeRef Interpreter::Produce(const String &source, Vector<String> &errors)
{
	NodeRef program = parser.Produce(source);
	errors = parser.GetErrors();

	return program;
}

void Interpreter::Run(const NodeRef &program, const Vector<String> &args)
{
	arena.Clear();

	breakFlag = false;
	continueFlag = false;
	aborted = false;
	depth = 0;
	steps = 0;

	Environment *environment = arena.Create(nullptr);
	environment->DeclareDefaults();

	String error;
	ValueRef argv = environment->LookupVariable("args", error);

	for (const String &arg : args)
	{
		argv->elements.push_back(MakeString(arg));
	}

	Interpret(program, environment);

	arena.Clear();
}

ValueRef Interpreter::Interpret(const NodeRef &node, Environment *environment)
{
	if (aborted || !node)
	{
		return MakeNull();
	}

	++steps;

	if (stepLimit != 0 && steps > stepLimit)
	{
		Fail("Program exceeded its step budget and was stopped.");
		return MakeNull();
	}

	switch (node->kind)
	{
		case NodeKind::Program:
			return InterpretProgram(node, environment);
		case NodeKind::VariableDeclaration:
			return InterpretVariableDeclaration(node, environment);
		case NodeKind::FunctionDeclaration:
			return InterpretFunctionDeclaration(node, environment);
		case NodeKind::PrintStatement:
			return InterpretPrint(node, environment);
		case NodeKind::ToggleStatement:
			return InterpretToggle(node, environment);
		case NodeKind::SplitStatement:
			return InterpretSplit(node, environment);
		case NodeKind::LengthStatement:
			return InterpretLength(node, environment);
		case NodeKind::ReadStatement:
			return InterpretRead(node, environment);
		case NodeKind::WriteStatement:
			return InterpretWrite(node, environment);
		case NodeKind::ReturnStatement:
			return InterpretReturn(node, environment);
		case NodeKind::ExecStatement:
			return InterpretExec(node, environment);
		case NodeKind::SpawnStatement:
			return InterpretSpawn(node, environment);
		case NodeKind::IfStatement:
			return InterpretIf(node, environment);
		case NodeKind::WhileStatement:
			return InterpretWhile(node, environment);
		case NodeKind::ForStatement:
			return InterpretFor(node, environment);
		case NodeKind::BreakStatement:
			breakFlag = true;
			return MakeNull();
		case NodeKind::ContinueStatement:
			continueFlag = true;
			return MakeNull();
		case NodeKind::NumericLiteral:
			return MakeNumber(node->number);
		case NodeKind::StringLiteral:
			return MakeString(node->symbol);
		case NodeKind::Identifier:
			return InterpretIdentifier(node, environment);
		case NodeKind::ObjectLiteral:
			return InterpretObject(node, environment);
		case NodeKind::ArrayLiteral:
			return InterpretArray(node, environment);
		case NodeKind::MemberExpression:
			return InterpretMember(node, environment);
		case NodeKind::AssignmentExpression:
			return InterpretAssignment(node, environment);
		case NodeKind::BinaryExpression:
			return InterpretBinary(node, environment);
		case NodeKind::UnaryExpression:
			return InterpretUnary(node, environment);
		case NodeKind::LogicalExpression:
			return InterpretLogical(node, environment);
		case NodeKind::CallExpression:
			return InterpretCall(node, environment);
		case NodeKind::Empty:
			return MakeNull();
	}

	Fail("Your code uses something that is not supported yet.");

	return MakeNull();
}

ValueRef Interpreter::InterpretProgram(const NodeRef &node, Environment *environment)
{
	ValueRef last = MakeNull();

	for (const NodeRef &statement : node->body)
	{
		last = Interpret(statement, environment);

		if (aborted)
		{
			break;
		}
	}

	return last;
}

ValueRef Interpreter::InterpretVariableDeclaration(const NodeRef &node, Environment *environment)
{
	ValueRef value = node->value ? Interpret(node->value, environment) : MakeNull();

	String error;
	ValueRef declared = environment->DeclareVariable(node->identifier, value, node->constant, error);

	if (!error.empty())
	{
		Fail(error);
	}

	return declared;
}

ValueRef Interpreter::InterpretFunctionDeclaration(const NodeRef &node, Environment *environment)
{
	Ref<Value> function = std::make_shared<Value>();
	function->type = ValueType::Function;
	function->name = node->identifier;
	function->parameters = node->parameters;
	function->body = node->body;
	function->declarationEnvironment = environment;

	String error;
	ValueRef declared = environment->DeclareVariable(node->identifier, function, true, error);

	if (!error.empty())
	{
		Fail(error);
	}

	return declared;
}

ValueRef Interpreter::InterpretPrint(const NodeRef &node, Environment *environment)
{
	ValueRef value = Interpret(node->value, environment);

	if (host != nullptr)
	{
		host->Print(Stringify(value));
	}

	return value;
}

ValueRef Interpreter::InterpretToggle(const NodeRef &node, Environment *environment)
{
	ValueRef name = Interpret(node->name, environment);
	ValueRef value = Interpret(node->value, environment);
	ValueRef active = Interpret(node->active, environment);

	if (host != nullptr)
	{
		host->Toggle(Stringify(name), Stringify(value), IsTruthy(active));
	}

	return MakeNull();
}

ValueRef Interpreter::InterpretSplit(const NodeRef &node, Environment *environment)
{
	ValueRef value = Interpret(node->value, environment);
	ValueRef delimiter = Interpret(node->delimiter, environment);

	const String source = Stringify(value);
	const String separator = Stringify(delimiter);

	Vector<ValueRef> parts;

	if (separator.empty())
	{
		for (const char character : source)
		{
			parts.push_back(MakeString(String(1, character)));
		}
	}
	else
	{
		std::size_t start = 0;
		std::size_t found = source.find(separator, start);

		while (found != String::npos)
		{
			parts.push_back(MakeString(source.substr(start, found - start)));
			start = found + separator.size();
			found = source.find(separator, start);
		}

		parts.push_back(MakeString(source.substr(start)));
	}

	if (!node->property || node->property->kind != NodeKind::Identifier)
	{
		Fail("split expects an identifier as its third argument.");
		return MakeNull();
	}

	String error;
	environment->AssignVariable(node->property->symbol, MakeArray(std::move(parts)), error);

	if (!error.empty())
	{
		Fail(error);
	}

	return MakeNull();
}

ValueRef Interpreter::InterpretLength(const NodeRef &node, Environment *environment)
{
	ValueRef value = Interpret(node->value, environment);

	if (value && value->type == ValueType::Array)
	{
		return MakeNumber(static_cast<double>(value->elements.size()));
	}

	return MakeNumber(static_cast<double>(Stringify(value).size()));
}

ValueRef Interpreter::InterpretRead(const NodeRef &node, Environment *environment)
{
	ValueRef value = Interpret(node->value, environment);

	String source;

	if (host == nullptr || !host->ReadFile(Stringify(value), source))
	{
		Fail("File " + Stringify(value) + " doesn't exist");
		return MakeNull();
	}

	return MakeString(source);
}

ValueRef Interpreter::InterpretWrite(const NodeRef &node, Environment *environment)
{
	ValueRef file = Interpret(node->file, environment);
	ValueRef source = Interpret(node->source, environment);

	if (host != nullptr)
	{
		host->WriteFile(Stringify(file), Stringify(source));
	}

	return MakeNull();
}

ValueRef Interpreter::InterpretReturn(const NodeRef &node, Environment *environment)
{
	return MakeReturn(Interpret(node->value, environment));
}

ValueRef Interpreter::InterpretExec(const NodeRef &node, Environment *environment)
{
	ValueRef value = Interpret(node->value, environment);

	const String command = Stringify(value);
	const String name = command.substr(0, command.find(' '));

	String source;

	if (host == nullptr || !host->ReadFile(name, source))
	{
		Fail("File " + name + " doesn't exist");
		return MakeNull();
	}

	if (depth >= MAX_DEPTH)
	{
		Fail("exec nested too deeply.");
		return MakeNull();
	}

	Parser nested;
	NodeRef program = nested.Produce(source);

	if (!nested.GetErrors().empty())
	{
		for (const String &error : nested.GetErrors())
		{
			Fail(error);
		}

		return MakeNull();
	}

	++depth;
	ValueRef result = Interpret(program, environment);
	--depth;

	return result;
}

ValueRef Interpreter::InterpretSpawn(const NodeRef &node, Environment *environment)
{
	ValueRef value = Interpret(node->value, environment);

	if (host != nullptr)
	{
		host->Spawn(Stringify(value));
	}

	return MakeNull();
}

bool Interpreter::RunBody(const Vector<NodeRef> &body, Environment *environment, ValueRef &result)
{
	for (const NodeRef &statement : body)
	{
		result = Interpret(statement, environment);

		if (aborted)
		{
			return false;
		}

		if (result && result->type == ValueType::Return)
		{
			return false;
		}

		if (breakFlag || continueFlag)
		{
			return false;
		}
	}

	return true;
}

ValueRef Interpreter::InterpretIf(const NodeRef &node, Environment *environment)
{
	ValueRef condition = Interpret(node->condition, environment);
	ValueRef result = MakeNull();

	if (IsTruthy(condition))
	{
		RunBody(node->thenBranch, environment, result);
	}
	else
	{
		RunBody(node->elseBranch, environment, result);
	}

	if (result && result->type == ValueType::Return)
	{
		return result;
	}

	return MakeNull();
}

ValueRef Interpreter::InterpretWhile(const NodeRef &node, Environment *environment)
{
	ValueRef result = MakeNull();

	while (!aborted && IsTruthy(Interpret(node->condition, environment)))
	{
		RunBody(node->body, environment, result);

		if (aborted)
		{
			break;
		}

		if (result && result->type == ValueType::Return)
		{
			return result;
		}

		if (breakFlag)
		{
			breakFlag = false;
			break;
		}

		continueFlag = false;
	}

	return MakeNull();
}

ValueRef Interpreter::InterpretFor(const NodeRef &node, Environment *environment)
{
	Interpret(node->initializer, environment);

	ValueRef result = MakeNull();

	while (!aborted && IsTruthy(Interpret(node->condition, environment)))
	{
		RunBody(node->body, environment, result);

		if (aborted)
		{
			break;
		}

		if (result && result->type == ValueType::Return)
		{
			return result;
		}

		if (breakFlag)
		{
			breakFlag = false;
			break;
		}

		continueFlag = false;

		Interpret(node->increment, environment);
	}

	return MakeNull();
}

ValueRef Interpreter::InterpretIdentifier(const NodeRef &node, Environment *environment)
{
	String error;
	ValueRef value = environment->LookupVariable(node->symbol, error);

	if (!error.empty())
	{
		Fail(error);
	}

	return value;
}

ValueRef Interpreter::InterpretObject(const NodeRef &node, Environment *environment)
{
	ValueRef object = MakeObject();

	for (const Property &property : node->properties)
	{
		if (property.value)
		{
			object->properties[property.key] = Interpret(property.value, environment);
			continue;
		}

		String error;
		object->properties[property.key] = environment->LookupVariable(property.key, error);

		if (!error.empty())
		{
			Fail(error);
		}
	}

	return object;
}

ValueRef Interpreter::InterpretArray(const NodeRef &node, Environment *environment)
{
	Vector<ValueRef> elements;
	elements.reserve(node->elements.size());

	for (const NodeRef &element : node->elements)
	{
		elements.push_back(Interpret(element, environment));
	}

	return MakeArray(std::move(elements));
}

ValueRef Interpreter::InterpretMember(const NodeRef &node, Environment *environment)
{
	ValueRef object = Interpret(node->object, environment);

	if (!object)
	{
		Fail("Object value is null");
		return MakeNull();
	}

	if (object->type == ValueType::Object)
	{
		String key;

		if (node->computed)
		{
			key = Stringify(Interpret(node->property, environment));
		}
		else if (node->property)
		{
			key = node->property->symbol;
		}

		const auto found = object->properties.find(key);

		if (found == object->properties.end())
		{
			return MakeNull();
		}

		return found->second;
	}

	if (object->type == ValueType::Array || object->type == ValueType::String)
	{
		if (!node->computed)
		{
			Fail("Index must be a number");
			return MakeNull();
		}

		ValueRef indexValue = Interpret(node->property, environment);

		if (!indexValue || indexValue->type != ValueType::Number)
		{
			Fail("Index must be a number");
			return MakeNull();
		}

		const std::size_t size =
			object->type == ValueType::Array ? object->elements.size() : object->string.size();

		double index = indexValue->number;

		if (index < 0.0)
		{
			index += static_cast<double>(size);
		}

		if (index < 0.0 || index >= static_cast<double>(size))
		{
			return MakeNull();
		}

		const std::size_t offset = static_cast<std::size_t>(index);

		if (object->type == ValueType::Array)
		{
			return object->elements[offset];
		}

		return MakeString(String(1, object->string[offset]));
	}

	Fail("Cannot read property of value");

	return MakeNull();
}

ValueRef Interpreter::InterpretAssignment(const NodeRef &node, Environment *environment)
{
	if (!node->assigne || node->assigne->kind != NodeKind::Identifier)
	{
		Fail("Invalid assignment expression");
		return MakeNull();
	}

	String error;
	ValueRef value = environment->AssignVariable(node->assigne->symbol, Interpret(node->value, environment), error);

	if (!error.empty())
	{
		Fail(error);
	}

	return value;
}

ValueRef Interpreter::InterpretBinary(const NodeRef &node, Environment *environment)
{
	ValueRef left = Interpret(node->left, environment);
	ValueRef right = Interpret(node->right, environment);

	if (node->op == "+" && BothStrings(left, right))
	{
		return MakeString(Stringify(left) + Stringify(right));
	}

	const double leftNumber = NumberOf(left);
	const double rightNumber = NumberOf(right);

	if (node->op == "+")
	{
		return MakeNumber(leftNumber + rightNumber);
	}

	if (node->op == "-")
	{
		return MakeNumber(leftNumber - rightNumber);
	}

	if (node->op == "*")
	{
		return MakeNumber(leftNumber * rightNumber);
	}

	if (node->op == "/")
	{
		if (rightNumber == 0.0)
		{
			Fail("Division by zero.");
			return MakeNull();
		}

		return MakeNumber(leftNumber / rightNumber);
	}

	if (node->op == "%")
	{
		if (rightNumber == 0.0)
		{
			Fail("Division by zero.");
			return MakeNull();
		}

		return MakeNumber(std::fmod(leftNumber, rightNumber));
	}

	return MakeNumber(0.0);
}

ValueRef Interpreter::InterpretUnary(const NodeRef &node, Environment *environment)
{
	ValueRef right = Interpret(node->right, environment);

	if (node->op == "-")
	{
		return MakeNumber(-NumberOf(right));
	}

	if (node->op == "!")
	{
		return MakeBoolean(!IsTruthy(right));
	}

	return MakeNull();
}

ValueRef Interpreter::InterpretLogical(const NodeRef &node, Environment *environment)
{
	ValueRef left = Interpret(node->left, environment);
	ValueRef right = Interpret(node->right, environment);

	if (node->op == "==")
	{
		return MakeBoolean(Equals(left, right));
	}

	if (node->op == "!=")
	{
		return MakeBoolean(!Equals(left, right));
	}

	if (BothStrings(left, right))
	{
		const String leftString = Stringify(left);
		const String rightString = Stringify(right);

		if (node->op == "<")
		{
			return MakeBoolean(leftString < rightString);
		}

		if (node->op == ">")
		{
			return MakeBoolean(leftString > rightString);
		}

		if (node->op == "<=")
		{
			return MakeBoolean(leftString <= rightString);
		}

		if (node->op == ">=")
		{
			return MakeBoolean(leftString >= rightString);
		}
	}

	const double leftNumber = NumberOf(left);
	const double rightNumber = NumberOf(right);

	if (node->op == "<")
	{
		return MakeBoolean(leftNumber < rightNumber);
	}

	if (node->op == ">")
	{
		return MakeBoolean(leftNumber > rightNumber);
	}

	if (node->op == "<=")
	{
		return MakeBoolean(leftNumber <= rightNumber);
	}

	if (node->op == ">=")
	{
		return MakeBoolean(leftNumber >= rightNumber);
	}

	return MakeBoolean(false);
}

ValueRef Interpreter::InterpretCall(const NodeRef &node, Environment *environment)
{
	Vector<ValueRef> args;
	args.reserve(node->args.size());

	for (const NodeRef &arg : node->args)
	{
		args.push_back(Interpret(arg, environment));
	}

	ValueRef function = Interpret(node->caller, environment);

	if (!function || function->type != ValueType::Function)
	{
		Fail("Cannot call value that is not a function.");
		return MakeNull();
	}

	if (function->parameters.size() != args.size())
	{
		Fail(
			"Expected " + std::to_string(function->parameters.size()) + " arguments, but got " +
			std::to_string(args.size()));
		return MakeNull();
	}

	if (depth >= MAX_DEPTH)
	{
		Fail("Call stack exceeded.");
		return MakeNull();
	}

	Environment *scope = arena.Create(function->declarationEnvironment);

	for (std::size_t index = 0; index < function->parameters.size(); ++index)
	{
		String error;
		scope->DeclareVariable(function->parameters[index], args[index], false, error);

		if (!error.empty())
		{
			Fail(error);
			return MakeNull();
		}
	}

	++depth;

	ValueRef result = MakeNull();

	for (const NodeRef &statement : function->body)
	{
		result = Interpret(statement, scope);

		if (aborted)
		{
			break;
		}

		if (result && result->type == ValueType::Return)
		{
			break;
		}
	}

	--depth;

	if (result && result->type == ValueType::Return)
	{
		return result->inner ? result->inner : MakeNull();
	}

	return MakeNull();
}
} // namespace TGX::Shell
