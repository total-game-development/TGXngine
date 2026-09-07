#include "ShellScene.h"
#include <nlohmann/json.hpp>
#include "Enums.h"
#include "Renderer.h"
#include "Window.h"
#include "io/Loader.h"

using namespace nlohmann;

namespace TGX
{
namespace
{
Unique<Loader> loader;

void HandleToggle(const char *name, const char *value, bool active)
{
	Log::Print(String("toggle ") + name + " " + value + (active ? " on" : " off"));
}
} // namespace

ShellScene::ShellScene()
{
	Log::Success("Shell Scene Created");
}

ShellScene::~ShellScene()
{
	Log::Success("Deleted Shell Scene");
}

void ShellScene::Load()
{
	json requirements;
	requirements["required_libraries"] = json::array();
	requirements["required_libraries"].push_back({{"type", "shell"}, {"name", "modules/Shell"}});

	loader = std::make_unique<Loader>();
	loader->AssignGameDLLs(requirements);
	loader->AssignShell();

	loaded = loader->GetShell() != nullptr;
}

void ShellScene::Init()
{
	if (!loaded)
	{
		Load();
	}

	if (!loaded)
	{
		Log::Error("Shell module unavailable");
		return;
	}

	loader->GetShell()->Awake("shell");
	loader->GetShell()->SetToggleHandler(&HandleToggle);
	loader->GetShell()->Create();
}

void ShellScene::Update()
{
	if (!loaded)
	{
		return;
	}

	loader->GetShell()->Update();

	if (loader->GetShell()->ShouldClose())
	{
		Renderer::GetInstance().LoadScene(SceneType::Intro);
	}
}

void ShellScene::Draw()
{
	if (loaded)
	{
		loader->GetShell()->Draw();
	}
}

void ShellScene::Click()
{
	if (loaded)
	{
		loader->GetShell()->Click();
	}
}

void ShellScene::Text(unsigned int codepoint)
{
	if (loaded)
	{
		loader->GetShell()->Text(codepoint);
	}
}

void ShellScene::Key(int code)
{
	if (loaded)
	{
		loader->GetShell()->Key(code);
	}
}

void ShellScene::RightClick()
{
}

void ShellScene::Release()
{
}

void ShellScene::Close()
{
	if (loaded)
	{
		loader->GetShell()->Clear();
	}
}

void ShellScene::Free()
{
}
} // namespace TGX
