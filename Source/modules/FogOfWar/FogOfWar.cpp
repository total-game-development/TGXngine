#include "FogOfWar.h"
#include "Logs.h"
#include "module_interface.h"

namespace TGX
{

extern "C"
{
	MODULE_API void Init()
	{
		Log::Info("FogOfWar module loaded");
		GetState().Init();
	}

	MODULE_API void Destroy()
	{
		Log::Print("FogOfWar module destroyed");
	}

	MODULE_API void Update()
	{
		GetState().Update();
	}

	MODULE_API void Draw()
	{
		GetState().Draw();
	}
}

} // namespace TGX
