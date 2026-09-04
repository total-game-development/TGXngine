#include "Renderer.h"
#include <any>
#include <iostream>
#include <regex>
#include <typeinfo>
#include <utility>
#include "Controller.h"
#include "Enums.h"
#include "Globals.h"
#include "Keyboard.h"
#include "Logs.h"
#include "Mouse.h"
#include "Physics.h"
#include "SFML/Window/Event.hpp"
#include "Scene/Game.h"
#include "Scene/Intro.h"
#include "Scene/Skirmish.h"
#include "StringUtils.hpp"
#include "Window.h"

#ifdef LoadCursor
#undef LoadCursor
#endif

namespace TGX
{
Renderer::Renderer()
{
	Log::Info("Create Renderer");
	WorldState &world = WorldState::GetInstance();
	Log::Info("IsProduction " + std::to_string(world.IsProduction()));

	Window::GetInstance();

	scenes.insert({SceneType::Intro, std::make_shared<Intro>()});
	scenes.insert({SceneType::Skirmish, std::make_shared<Skirmish>()});
	scenes.insert({SceneType::Game, std::make_shared<Game>()});

	functions[UIAction::Log] = &Renderer::Log;
	functions[UIAction::LoadScene] = &Renderer::LoadScene;
	functions[UIAction::Print] = &Renderer::Print;
	functions[UIAction::AddGameItem] = &Renderer::AddGameItem;
	functions[UIAction::RemoveGameItem] = &Renderer::RemoveGameItem;
	functions[UIAction::GameOver] = &Renderer::GameOver;
	functions[UIAction::Cancel] = &Renderer::Cancel;

	world.SetCanvasSize(Globals::canvasWidth, Globals::canvasHeight);
}

Renderer::~Renderer() = default;

Renderer &Renderer::GetInstance()
{
	static Renderer renderer;
	return renderer;
}

void Renderer::Log(const Any &msg) const
{
	if (msg.type() == typeid(String))
	{
		auto message = std::any_cast<String>(msg);

		if (message != "none")
		{
			Log::Print(message);
		}
	}
}

void Renderer::Start()
{
	Window &window = Window::GetInstance();
	window.Start();

	cursorTextures.reserve(5);
	cursors.reserve(5);
	LoadCursor("Resources/images/cursor/cursor.png");
	LoadCursor("Resources/images/cursor/selection.png", true);
	LoadCursor("Resources/images/cursor/target.png", true);
	LoadCursor("Resources/images/cursor/extract.png", true);
	LoadCursor("Resources/images/cursor/cursor-load.png", true);

	sf::Clock clock;
	sf::Clock fpsUpdateClock;
	sf::Time previousTime = clock.getElapsedTime();
	sf::Time currentTime;

	float frameCount = 0;

	Mouse &mouse = Mouse::GetInstance();
	Keyboard &keyboard = Keyboard::GetInstance();
	Controller &controller = Controller::GetInstance();
	WorldState &world = WorldState::GetInstance();

	world.SetTargetFPS(Globals::targetFPS);

	std::function<void(sf::Event)> mouseCallback = [&mouse, this, &window](sf::Event event) {
		switch (event.type)
		{
			case sf::Event::MouseMoved:
				{
					sf::Vector2i pixel(event.mouseMove.x, event.mouseMove.y);
					sf::Vector2f world_pos = window.PixelToCoords(pixel);
					mouse.Moved(world_pos.x, world_pos.y);
					break;
				}

			case sf::Event::MouseButtonPressed:
				if (event.mouseButton.button == sf::Mouse::Left)
				{
					mouse.Click();
					scene->Click();
				}
				else if (event.mouseButton.button == sf::Mouse::Right)
				{
					mouse.RightClick();
					scene->RightClick();
				}
				break;

			case sf::Event::MouseButtonReleased:
				if (event.mouseButton.button == sf::Mouse::Left)
				{
					mouse.Release();
					scene->Release();
				}
				break;

			default:
				break;
		};
	};

	std::function<void(sf::Event)> keyboardCallback = [&keyboard](sf::Event event) {
		keyboard.KeyPressed(event.key.code);
	};

	window.SetEventCallbacks(keyboardCallback, mouseCallback);

	while (!window.ShouldClose() && !world.IsClosed())
	{
		float dt = clock.restart().asSeconds();

		world.SetDeltaTime(std::min(dt, 0.05f));

		frameCount++;
		if (fpsUpdateClock.getElapsedTime().asSeconds() >= 0.5f)
		{
			float averageFPS = frameCount / fpsUpdateClock.restart().asSeconds();
			world.SetFPS(averageFPS);
			frameCount = 0;
		}

		window.Update();
		if (window.ShouldClose())
		{
			break;
		}

		// =========================
		// Update logic
		// =========================
		RunFunctions();
		scene->Update();
		mouse.Update();
		controller.Update();

		// =========================
		// Controller → Scene Input
		// =========================
		static bool wasSelecting = false;

		bool isSelecting = controller.IsActionHeld(ControllerAction::Select);

		if (isSelecting && !wasSelecting)
		{
			mouse.Click();
			scene->Click();
		}
		else if (!isSelecting && wasSelecting)
		{
			mouse.Release();
			scene->Release();
		}

		wasSelecting = isSelecting;

		if (controller.IsActionPressed(ControllerAction::Action))
		{
			mouse.RightClick();
			scene->RightClick();
		}

		// =========================
		// Render
		// =========================
		window.Clear();
		scene->Draw();
		mouse.Draw();
		mouse.DrawCursor(cursors);
		window.Display();
	}

	scene->Close();
	window.Close();
}

void Renderer::LoadScene(Any inScene)
{
	SceneType type = SceneType::Unknown;
	if (inScene.type() == typeid(String))
	{
		type = SceneTypeFromString(std::any_cast<String>(inScene));
	}
	else if (inScene.type() == typeid(SceneType))
	{
		type = std::any_cast<SceneType>(inScene);
	}

	if (this->scene != nullptr)
	{
		this->scene->Close();
		this->scene->Free();
	}

	this->scene = scenes[type];

	this->scene->Init();
}

void Renderer::Print(const Any &message)
{
	Log::Print(std::any_cast<String>(message));
}

void Renderer::AddGameItem(Any item)
{
	Log::Info("Add Game Item");
	auto addGameItemCommand = std::any_cast<String>(item);

	std::regex word_reg(R"(([a-zA-Z]+([_-][a-zA-Z]+)*))");
	addGameItemCommand = std::regex_replace(addGameItemCommand, word_reg, R"("$&")");

	std::regex true_reg(R"("true")");
	addGameItemCommand = std::regex_replace(addGameItemCommand, true_reg, "true");

	std::regex false_reg(R"("false")");
	addGameItemCommand = std::regex_replace(addGameItemCommand, false_reg, "false");

	Log::Print(addGameItemCommand);

	json json_command = json::parse("{" + addGameItemCommand + "}");

	std::static_pointer_cast<Game>(scenes[SceneType::Game])->AddGameItem(json_command);

	String command = json_command["command"];
	String name = json_command["name"];

	Log::Print("Command: " + command);
	Log::Print("Name: " + name);
}

void Renderer::GameOver(Any outcome)
{
	auto result = std::any_cast<String>(outcome);

	Log::Success("GameOver: " + result);

	auto game = std::static_pointer_cast<Game>(scenes[SceneType::Game]);

	if (game)
	{
		game->SetOutcome(result);
	}
}

void Renderer::RemoveGameItem(Any item)
{
	Log::Print("RemoveGameItem");

	auto removeGameItemCommand = std::any_cast<String>(item);

	std::regex comma_reg("([a-zA-Z]+)");
	removeGameItemCommand = std::regex_replace(removeGameItemCommand, comma_reg, R"("$&")");

	std::regex true_reg("(\"true\")");
	removeGameItemCommand = std::regex_replace(removeGameItemCommand, true_reg, "true");

	std::regex false_reg("(\"false\")");
	removeGameItemCommand = std::regex_replace(removeGameItemCommand, false_reg, "false");

	Log::Print(removeGameItemCommand);

	json json_command = json::parse("{" + removeGameItemCommand + "}");

	std::static_pointer_cast<Game>(scenes[SceneType::Game])->RemoveGameItem(json_command);
}

void Renderer::RunFunctions()
{
	WorldState &world = WorldState::GetInstance();

	auto events = std::move(world.gameEvents);
	world.gameEvents.clear();

	for (auto &gameEvent : events)
	{
		UIAction action = gameEvent.first;
		String value = gameEvent.second;

		auto it = functions.find(action);
		if (it != functions.end())
		{
			it->second(Renderer::GetInstance(), value);
		}
	}
}

void Renderer::LoadCursor(const String &file, bool center)
{
	cursorTextures.emplace_back();
	cursorTextures.back().loadFromFile(file);

	cursors.emplace_back(cursorTextures.back());

	if (center)
	{
		const sf::FloatRect bounds = cursors.back().getLocalBounds();
		cursors.back().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
	}
}

void Renderer::Cancel(const Any &item)
{
}
} // namespace TGX
