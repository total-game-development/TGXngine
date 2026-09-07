#pragma once

#include "Core.h"
#include "Scene.h"

namespace TGX
{
class ShellScene : public Scene
{
private:
	bool loaded = false;

	void Load();

public:
	ShellScene();
	~ShellScene() override;

	void Init() override;
	void Update() override;
	void Draw() override;
	void Click() override;
	void RightClick() override;
	void Release() override;
	void Close() override;
	void Free() override;

	void Text(unsigned int codepoint);
	void Key(int code);
};
} // namespace TGX
