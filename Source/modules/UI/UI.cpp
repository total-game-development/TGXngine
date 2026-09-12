#include "Logs.h"
#include "Portal.h"
#include "module_interface.h"

namespace TGX::UI
{
namespace
{
Unique<Portal> portal;
} // namespace

extern "C"
{
	MODULE_API void Init()
	{
		Log::Success("UI initialized");
	}

	MODULE_API void Awake(const String &name)
	{
		if (!portal)
		{
			portal = std::make_unique<Portal>();
		}

		portal->Load("Resources/" + name + ".json");
		portal->Awake(name);

		Log::Success("UI created: " + name);
	}

	MODULE_API void Create()
	{
		if (portal)
		{
			portal->Start();
		}
	}

	MODULE_API void Update()
	{
		if (portal)
		{
			portal->Update();
		}
	}

	MODULE_API void Draw()
	{
		if (portal)
		{
			portal->Draw();
		}
	}

	MODULE_API bool Click()
	{
		return portal && portal->Press();
	}

	MODULE_API void Release()
	{
		if (portal)
		{
			portal->Release();
		}
	}

	MODULE_API bool Text(unsigned int codepoint)
	{
		return portal && portal->Character(codepoint);
	}

	MODULE_API bool Key(int code)
	{
		return portal && portal->Key(code);
	}

	MODULE_API bool ConsoleBounds(float *x, float *y, float *width, float *height)
	{
		if (!portal)
		{
			return false;
		}

		sf::FloatRect bounds;

		if (!portal->Console(bounds))
		{
			return false;
		}

		if (x != nullptr) { *x = bounds.left; }
		if (y != nullptr) { *y = bounds.top; }
		if (width != nullptr) { *width = bounds.width; }
		if (height != nullptr) { *height = bounds.height; }

		return true;
	}

	MODULE_API bool IsVisible()
	{
		return portal && portal->IsVisible();
	}

	MODULE_API bool IsPaused()
	{
		return portal && portal->IsPaused();
	}

	MODULE_API void Clear()
	{
		if (portal)
		{
			portal->Hide();
		}

		Log::Success("Clear UI");
	}

	MODULE_API void Delete()
	{
		if (portal)
		{
			portal->Clear();
			portal.reset();
		}

		Log::Clean("Delete UI");
	}

	MODULE_API void Destroy(const String &name)
	{
		Log::Clean("Destroy UI " + name);
	}
}
} // namespace TGX::UI
