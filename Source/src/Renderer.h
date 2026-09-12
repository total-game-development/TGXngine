#pragma once

#include <SFML/Graphics.hpp>
#include <nlohmann/json.hpp>
#include <cstdlib>
#include "Core.h"
#include "Enums.h"
#include "Scene/Scene.h"

using namespace nlohmann;

namespace TGX
{
class Renderer
{
public:
	// Runs one queued action now. A networked match replays an action the
	// server has stamped through it, rather than the one it queued locally.
	void RunAction(UIAction action, const String &value);

	Map<UIAction, Function<void(Renderer &, Any)>> functions;
	Map<SceneType, Ref<Scene>> scenes;
	Ref<Scene> scene;
	Vector<sf::Sprite> cursors;
	Vector<sf::Texture> cursorTextures;

	static Renderer &GetInstance();

	void Log(const Any &msg) const;
	void Start();
	void LoadScene(Any scene);
	void Print(const Any &message);
	void AddGameItem(Any item);
	void RemoveGameItem(Any item);
	void GameOver(Any outcome);
	void Cancel(const Any &item);

	Renderer(const Renderer &) = delete;
	Renderer &operator=(const Renderer &) = delete;

	Renderer(Renderer &&) = delete;
	Renderer &operator=(Renderer &&) = delete;

private:
	void RunFunctions();
	void LoadCursor(const String &file, bool center = false);

	Renderer();
	~Renderer();
};
} // namespace TGX
