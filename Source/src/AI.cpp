#include "AI.h"
#include "Logs.h"

namespace TGX
{
AI::AI(
	FNPTR_AI_AWAKE awake,
	FNPTR_AI_START start,
	FNPTR_AI_UPDATE update,
	FNPTR_AI_CLEAR clear,
	FNPTR_AI_DELETE _delete)
{
	Log::Info("AI Create");

	this->awake = awake;
	this->start = start;
	this->update = update;
	this->clear = clear;
	this->_delete = _delete;

	Log::Success("AI Created");
}

AI::~AI()
{
	Log::Info("Delete AI");
}

void AI::Awake(const String &name, const String &level)
{
	Log::Info("AI Awake " + name);

	awake(name, level);
}

void AI::Start()
{
	Log::Info("AI Start");

	start();
}

void AI::Update()
{
	update();
}

void AI::Clear()
{
	clear();
}

void AI::Delete()
{
	Log::Info("Delete AI");

	_delete();
}
} // namespace TGX
