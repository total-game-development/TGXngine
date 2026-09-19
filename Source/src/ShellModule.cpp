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
	FNPTR_SHELL_SET_VIEWPORT inSetViewport,
	FNPTR_SHELL_IS_EDITING inIsEditing,
	FNPTR_SHELL_SHOULD_CLOSE inShouldClose,
	FNPTR_SHELL_SET_TOGGLE_HANDLER inSetToggleHandler,
	FNPTR_SHELL_SET_NETWORK inSetNetwork,
	FNPTR_SHELL_DELIVER inDeliver,
	FNPTR_SHELL_SET_PROCESS_HANDLER inSetProcessHandler,
	FNPTR_SHELL_CLEAR inClear,
	FNPTR_SHELL_DELETE inDelete)
	: awake(inAwake),
	  create(inCreate),
	  update(inUpdate),
	  draw(inDraw),
	  click(inClick),
	  text(inText),
	  key(inKey),
	  setViewport(inSetViewport),
	  isEditing(inIsEditing),
	  shouldClose(inShouldClose),
	  setToggleHandler(inSetToggleHandler),
	  setNetwork(inSetNetwork),
	  deliver(inDeliver),
	  setProcessHandler(inSetProcessHandler),
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

void ShellModule::SetViewport(const sf::FloatRect &bounds)
{
	if (setViewport)
	{
		setViewport(bounds.left, bounds.top, bounds.width, bounds.height);
	}
}

bool ShellModule::IsEditing()
{
	return isEditing ? isEditing() : false;
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

void ShellModule::SetNetwork(FNPTR_SHELL_SEND send, const String &identity)
{
	if (setNetwork)
	{
		setNetwork(send, identity.c_str());
	}
}

void ShellModule::ClearNetwork()
{
	if (setNetwork)
	{
		setNetwork(nullptr, nullptr);
	}
}

void ShellModule::Deliver(const String &message)
{
	if (deliver)
	{
		deliver(message.c_str());
	}
}

void ShellModule::SetProcessHandler(FNPTR_SHELL_LIST_PROCESSES list, FNPTR_SHELL_KILL_PROCESS kill)
{
	if (setProcessHandler)
	{
		setProcessHandler(list, kill);
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
