#include "FogOfWar.h"

namespace TGX
{

FogOfWar::FogOfWar(FNPTR_FOGOFWAR_UPDATE inFnUpdate, FNPTR_FOGOFWAR_DRAW inFnDraw)
	: fnUpdate(inFnUpdate), fnDraw(inFnDraw)
{
}

void FogOfWar::Update()
{
	if (fnUpdate)
	{
		fnUpdate();
	}
}

void FogOfWar::Draw()
{
	if (fnDraw)
	{
		fnDraw();
	}
}

} // namespace TGX
