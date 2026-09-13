#pragma once

#include "Core.h"

namespace TGX
{
using FNPTR_TRIGGERS_AWAKE = void (*)(String);
using FNPTR_TRIGGERS_START = void (*)();
using FNPTR_TRIGGERS_UPDATE = void (*)();
using FNPTR_TRIGGERS_CLEAR = void (*)();
using FNPTR_TRIGGERS_DELETE = void (*)();

class Triggers
{
private:
	FNPTR_TRIGGERS_AWAKE awake;
	FNPTR_TRIGGERS_START start;
	FNPTR_TRIGGERS_UPDATE update;
	FNPTR_TRIGGERS_CLEAR clear;
	FNPTR_TRIGGERS_DELETE _delete;

public:
	Triggers(
		FNPTR_TRIGGERS_AWAKE,
		FNPTR_TRIGGERS_START,
		FNPTR_TRIGGERS_UPDATE,
		FNPTR_TRIGGERS_CLEAR,
		FNPTR_TRIGGERS_DELETE);
	~Triggers() = default;

	void Awake(const String &name);
	void Start();
	void Update();
	void Clear();
	void Delete();
};
} // namespace TGX
