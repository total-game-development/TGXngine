#include "ShellModule.h"

namespace TGX
{
ShellModule::ShellModule(
	FNPTR_SHELL_AWAKE inAwake,
	FNPTR_SHELL_CREATE inCreate,
	FNPTR_SHELL_UPDATE inUpdate,
	FNPTR_SHELL_RENDER_WINDOW inDraw,
	FNPTR_SHELL_CLICK inClick,
	FNPTR_SHELL_TEXT inText,
	FNPTR_SHELL_KEY inKey,
	FNPTR_SHELL_SHOULD_CLOSE inShouldClose,
	FNPTR_SHELL_SET_TOGGLE_HANDLER inSetToggleHandler,
	FNPTR_SHELL_CLEAR inClear,
	FNPTR_SHELL_DELETE inDelete)
	: awake(inAwake),
	  create(inCreate),
	  update(inUpdate),
	  draw(inDraw),
	  click(inClick),
	  text(inText),
	  key(inKey),
	  shouldClose(inShouldClose),
	  setToggleHandler(inSetToggleHandler),
	  clear(inClear),
	  _delete(inDelete)
{
}

void ShellModule::Awake(const String &name)
{
	if (awake)
	{
		awake(name);
	}
}

void ShellModule::Create()
{
	if (create)
	{
		create();
	}
}

void ShellModule::Update()
{
	if (update)
	{
		update();
	}
}

void ShellModule::Draw()
{
	if (draw)
	{
		draw();
	}
}

void ShellModule::Click()
{
	if (click)
	{
		click();
	}
}

void ShellModule::Text(unsigned int codepoint)
{
	if (text)
	{
		text(codepoint);
	}
}

void ShellModule::Key(int code)
{
	if (key)
	{
		key(code);
	}
}

bool ShellModule::ShouldClose()
{
	return shouldClose ? shouldClose() : false;
}

void ShellModule::SetToggleHandler(void (*handler)(const char *, const char *, bool))
{
	if (setToggleHandler)
	{
		setToggleHandler(handler);
	}
}

void ShellModule::Clear()
{
	if (clear)
	{
		clear();
	}
}

void ShellModule::Delete()
{
	if (_delete)
	{
		_delete();
	}
}
} // namespace TGX
