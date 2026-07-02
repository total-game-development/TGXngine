#pragma once

namespace TGX
{
using FNPTR_FOGOFWAR_UPDATE = void (*)();
using FNPTR_FOGOFWAR_DRAW = void (*)();

class FogOfWar
{
private:
	FNPTR_FOGOFWAR_UPDATE fnUpdate = nullptr;
	FNPTR_FOGOFWAR_DRAW fnDraw = nullptr;

public:
	FogOfWar(FNPTR_FOGOFWAR_UPDATE inFnUpdate, FNPTR_FOGOFWAR_DRAW inFnDraw);

	void Update();
	void Draw();
};

} // namespace TGX
