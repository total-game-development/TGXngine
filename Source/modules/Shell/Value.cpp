#include "Value.h"
#include <cmath>
#include <sstream>

namespace TGX::Shell
{
ValueRef MakeNull()
{
	Ref<Value> value = std::make_shared<Value>();
	value->type = ValueType::Null;
	return value;
}

ValueRef MakeNumber(double number)
{
	Ref<Value> value = std::make_shared<Value>();
	value->type = ValueType::Number;
	value->number = number;
	return value;
}

ValueRef MakeBoolean(bool boolean)
{
	Ref<Value> value = std::make_shared<Value>();
	value->type = ValueType::Boolean;
	value->boolean = boolean;
	return value;
}

ValueRef MakeString(const String &string)
{
	Ref<Value> value = std::make_shared<Value>();
	value->type = ValueType::String;
	value->string = string;
	return value;
}

ValueRef MakeArray(Vector<ValueRef> elements)
{
	Ref<Value> value = std::make_shared<Value>();
	value->type = ValueType::Array;
	value->elements = std::move(elements);
	return value;
}

ValueRef MakeObject()
{
	Ref<Value> value = std::make_shared<Value>();
	value->type = ValueType::Object;
	return value;
}

ValueRef MakeReturn(ValueRef inner)
{
	Ref<Value> value = std::make_shared<Value>();
	value->type = ValueType::Return;
	value->inner = std::move(inner);
	return value;
}

bool IsTruthy(const ValueRef &value)
{
	if (!value)
	{
		return false;
	}

	switch (value->type)
	{
		case ValueType::Number:
			return value->number != 0.0;
		case ValueType::String:
			return !value->string.empty();
		case ValueType::Boolean:
			return value->boolean;
		case ValueType::Null:
			return false;
		case ValueType::Array:
			return !value->elements.empty();
		case ValueType::Object:
			return !value->properties.empty();
		default:
			return true;
	}
}

String Stringify(const ValueRef &value)
{
	if (!value)
	{
		return "null";
	}

	switch (value->type)
	{
		case ValueType::Null:
			return "null";

		case ValueType::Boolean:
			return value->boolean ? "true" : "false";

		case ValueType::String:
			return value->string;

		case ValueType::Number:
			{
				double integral = 0.0;

				if (std::modf(value->number, &integral) == 0.0 && std::abs(value->number) < 1e15)
				{
					return std::to_string(static_cast<long long>(integral));
				}

				std::ostringstream stream;
				stream << value->number;
				return stream.str();
			}

		case ValueType::Array:
			{
				String result = "[";

				for (std::size_t index = 0; index < value->elements.size(); ++index)
				{
					if (index > 0)
					{
						result += ",";
					}

					result += Stringify(value->elements[index]);
				}

				return result + "]";
			}

		case ValueType::Object:
			{
				String result = "{";
				bool first = true;

				for (const auto &property : value->properties)
				{
					if (!first)
					{
						result += ",";
					}

					result += property.first + ":" + Stringify(property.second);
					first = false;
				}

				return result + "}";
			}

		case ValueType::Function:
			return "function " + value->name;

		case ValueType::Return:
			return Stringify(value->inner);
	}

	return "null";
}
} // namespace TGX::Shell
