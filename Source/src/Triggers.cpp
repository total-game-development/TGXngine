#include "Triggers.h"
#include "Logs.h"

namespace TGX
{
Triggers::Triggers(
	FNPTR_TRIGGERS_AWAKE awake,
	FNPTR_TRIGGERS_START start,
	FNPTR_TRIGGERS_UPDATE update,
	FNPTR_TRIGGERS_CLEAR clear,
	FNPTR_TRIGGERS_DELETE _delete)
{
	this->awake = awake;
	this->start = start;
	this->update = update;
	this->clear = clear;
	this->_delete = _delete;
}

void Triggers::Awake(const String &name)
{
	Log::Info("Triggers::Awake");
	Log::Info("Triggers Name: " + name);

	awake(name);
}

void Triggers::Start()
{
	Log::Info("Triggers::Start");

	start();
}

void Triggers::Update()
{
	if (update)
	{
		update();
	}
}

void Triggers::Clear()
{
	clear();
}

void Triggers::Delete()
{
	_delete();
}
} // namespace TGX
