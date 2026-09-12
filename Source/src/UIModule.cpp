#include "UIModule.h"
#include "Logs.h"

namespace TGX
{
UIModule::UIModule(
	FNPTR_UI_AWAKE awake,
	FNPTR_UI_CREATE create,
	FNPTR_UI_UPDATE update,
	FNPTR_UI_RENDER_WINDOW draw,
	FNPTR_UI_CLICK click,
	FNPTR_UI_RELEASE release,
	FNPTR_UI_TEXT text,
	FNPTR_UI_KEY key,
	FNPTR_UI_CONSOLE_BOUNDS consoleBounds,
	FNPTR_UI_IS_VISIBLE isVisible,
	FNPTR_UI_IS_PAUSED isPaused,
	FNPTR_UI_CLEAR clear,
	FNPTR_UI_DELETE _delete)
{
	this->awake = awake;
	this->create = create;
	this->update = update;
	this->draw = draw;
	this->click = click;
	this->release = release;
	this->text = text;
	this->key = key;
	this->consoleBounds = consoleBounds;
	this->isVisible = isVisible;
	this->isPaused = isPaused;
	this->clear = clear;
	this->_delete = _delete;

	Log::Success("UI Module Created");
}

void UIModule::Awake(const String &name)
{
	if (awake)
	{
		awake(name);
	}
}

void UIModule::Create()
{
	if (create)
	{
		create();
	}
}

void UIModule::Update()
{
	if (update)
	{
		update();
	}
}

void UIModule::Draw()
{
	if (draw)
	{
		draw();
	}
}

bool UIModule::Click()
{
	return click != nullptr && click();
}

void UIModule::Release()
{
	if (release)
	{
		release();
	}
}

bool UIModule::Text(unsigned int codepoint)
{
	return text != nullptr && text(codepoint);
}

bool UIModule::Key(int code)
{
	return key != nullptr && key(code);
}

bool UIModule::ConsoleBounds(sf::FloatRect &bounds)
{
	return consoleBounds && consoleBounds(&bounds.left, &bounds.top, &bounds.width, &bounds.height);
}

bool UIModule::IsVisible()
{
	return isVisible != nullptr && isVisible();
}

bool UIModule::IsPaused()
{
	return isPaused != nullptr && isPaused();
}

void UIModule::Clear()
{
	if (clear)
	{
		clear();
	}
}

void UIModule::Delete()
{
	if (_delete)
	{
		_delete();
	}
}
} // namespace TGX
