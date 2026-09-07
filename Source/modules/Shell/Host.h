#pragma once

#include "Core.h"

namespace TGX::Shell
{
class Host
{
public:
	virtual ~Host() = default;

	virtual void Print(const String &message) = 0;
	virtual bool ReadFile(const String &name, String &source) = 0;
	virtual void WriteFile(const String &name, const String &source) = 0;
	virtual void Toggle(const String &name, const String &value, bool active) = 0;
	virtual void Spawn(const String &command) = 0;
};
} // namespace TGX::Shell
