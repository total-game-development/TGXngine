#pragma once

#include "Core.h"

namespace TGX
{
using FNPTR_AI_AWAKE = void (*)(const String &, const String &);
using FNPTR_AI_START = void (*)();
using FNPTR_AI_UPDATE = void (*)();
using FNPTR_AI_CLEAR = void (*)();
using FNPTR_AI_DELETE = void (*)();

class AI
{
private:
	FNPTR_AI_AWAKE awake;
	FNPTR_AI_START start;
	FNPTR_AI_UPDATE update;
	FNPTR_AI_CLEAR clear;
	FNPTR_AI_DELETE _delete;

public:
	AI(
		FNPTR_AI_AWAKE,
		FNPTR_AI_START,
		FNPTR_AI_UPDATE,
		FNPTR_AI_CLEAR,
		FNPTR_AI_DELETE);
	~AI();

	void Awake(const String &name, const String &level);
	void Start();
	void Update();
	void Clear();
	void Delete();
};
} // namespace TGX
