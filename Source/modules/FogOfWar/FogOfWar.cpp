#include "Logs.h"
#include "module_interface.h"

namespace TGX
{

extern "C"
{
	MODULE_API void Init()
	{
		Log::Info("FogOfWar module loaded");
	}

	MODULE_API void Destroy()
	{
		Log::Print("FogOfWar module destroyed");
	}

	MODULE_API void Update()
	{
	}

	MODULE_API void Draw()
	{
	}
}

} // namespace TGX
