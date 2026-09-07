#pragma once

#include <cstdint>
#include "Core.h"

namespace TGX::Shell
{
class Environment;
struct Node;

enum class ValueType : std::uint8_t
{
	Null,
	Number,
	Boolean,
	String,
	Array,
	Object,
	Function,
	Return
};

struct Value;
using ValueRef = Ref<Value>;

struct Value
{
	ValueType type = ValueType::Null;

	double number = 0.0;
	bool boolean = false;
	String string;

	Vector<ValueRef> elements;
	Map<String, ValueRef> properties;

	String name;
	Vector<String> parameters;
	Vector<Ref<Node>> body;
	Environment *declarationEnvironment = nullptr;

	ValueRef inner;
};

ValueRef MakeNull();
ValueRef MakeNumber(double value);
ValueRef MakeBoolean(bool value);
ValueRef MakeString(const String &value);
ValueRef MakeArray(Vector<ValueRef> elements);
ValueRef MakeObject();
ValueRef MakeReturn(ValueRef value);

bool IsTruthy(const ValueRef &value);
String Stringify(const ValueRef &value);
} // namespace TGX::Shell
