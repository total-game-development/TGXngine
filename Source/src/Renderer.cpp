#include "Renderer.h"
#include <any>
#include <iostream>
#include <regex>
#include <typeinfo>
#include <utility>
#include "Controller.h"
#include "Debug.h"
#include "Enums.h"
#include "FrameTrace.h"
#include "Globals.h"
#include "Keyboard.h"
#include "Logs.h"
#include "Mouse.h"
#include "MultiplayerSetup.h"
#include "Net/Session.h"
#include "Physics.h"
#include "SFML/Window/Event.hpp"
#include "Scene/Game.h"
#include "Scene/Intro.h"
#include "Scene/Multiplayer.h"
#include "Scene/ShellScene.h"
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
	scenes.insert({SceneType::Shell, std::make_shared<ShellScene>()});
	scenes.insert({SceneType::Multiplayer, std::make_shared<Multiplayer>(false)});
	scenes.insert({SceneType::Arena, std::make_shared<Multiplayer>(true)});

	functions[UIAction::Log] = &Renderer::Log;
	functions[UIAction::LoadScene] = &Renderer::LoadScene;
	functions[UIAction::Print] = &Renderer::Print;
	functions[UIAction::PlayerProduce] = &Renderer::Produce;
	functions[UIAction::PlayerPlace] = &Renderer::Place;
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

Ref<Game> Renderer::GetGame()
{
	return std::static_pointer_cast<Game>(scenes[SceneType::Game]);
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

	std::function<void(sf::Event)> keyboardCallback = [&keyboard, this](sf::Event event) {
		if (auto *shell = dynamic_cast<ShellScene *>(scene.get()))
		{
			shell->Key(static_cast<int>(event.key.code));
			return;
		}

		if (auto *game = dynamic_cast<Game *>(scene.get()))
		{
			if (game->Key(static_cast<int>(event.key.code)))
			{
				return;
			}
		}

		if (auto *multiplayer = dynamic_cast<Multiplayer *>(scene.get()))
		{
			if (multiplayer->Key(static_cast<int>(event.key.code)))
			{
				return;
			}
		}

		keyboard.KeyPressed(event.key.code);
	};

	std::function<void(sf::Event)> textCallback = [this](sf::Event event) {
		if (auto *shell = dynamic_cast<ShellScene *>(scene.get()))
		{
			shell->Text(event.text.unicode);
			return;
		}

		if (auto *game = dynamic_cast<Game *>(scene.get()))
		{
			game->Text(event.text.unicode);
		}
	};

	window.SetEventCallbacks(keyboardCallback, mouseCallback);
	window.SetTextCallback(textCallback);

	FrameTrace frames;

	while (!window.ShouldClose() && !world.IsClosed())
	{
		if (Debug::traceFrames)
		{
			frames.Begin();
		}

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

		if (Debug::traceFrames)
		{
			frames.Work();
		}

		window.Display();

		if (Debug::traceFrames)
		{
			frames.End(1000.0f / static_cast<float>(Globals::targetFPS));
		}
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

	// A unit is deployed from the building that made it and arrives with no
	// coordinates of its own. A building has nowhere to be put but where it was
	// asked for, so one that arrives without them would be raised at the origin
	// -- which is a corner of the map, and looks like a building that was placed
	// rather than one that was never placed at all.
	if (json_command.value("type", String{}) == "buildings" &&
		!(json_command.contains("x") && json_command.contains("y")))
	{
		Log::Error("Refusing to build " + json_command.value("name", String{"?"}) + ": no position was given");
		return;
	}

	std::static_pointer_cast<Game>(scenes[SceneType::Game])->AddGameItem(json_command);

	String command = json_command["command"];
	String name = json_command["name"];

	Log::Print("Command: " + command);
	Log::Print("Name: " + name);
}

void Renderer::Produce(const Any &request)
{
	const auto entry = std::any_cast<String>(request);

	WorldState &world = WorldState::GetInstance();

	const String team = StringField(entry, "team");
	const String key = StringField(entry, "key");
	const int cost = std::atoi(StringField(entry, "cost").c_str());

	// One of each at a time, as a sidebar button makes one at a time.
	bool accepted = world.FindProduction(team, key) == nullptr;

	if (!accepted)
	{
		Log::Warning("Already making " + key + " for " + team);
	}
	else if (!world.SpendTeamCash(team, cost))
	{
		accepted = false;

		Log::Warning("Purchase refused for " + team + ": $" + std::to_string(cost) + " of $" + std::to_string(world.GetTeamCash(team)));
	}
	else
	{
		ProductionOrder order;
		order.team = team;
		order.key = key;
		order.type = StringField(entry, "type");
		order.ticks = std::max(1, std::atoi(StringField(entry, "ticks").c_str()));

		world.productionOrders.push_back(order);
	}

	world.settledPurchases.push_back(entry + ",paid:" + (accepted ? "true" : "false"));
}

void Renderer::Place(const Any &request)
{
	const auto entry = std::any_cast<String>(request);

	WorldState &world = WorldState::GetInstance();

	const String team = StringField(entry, "team");
	const String key = StringField(entry, "key");

	// Only something made and paid for can be put down: a placement with no
	// finished order behind it builds nothing.
	const auto found = std::ranges::find_if(world.productionOrders, [&](const ProductionOrder &order) {
		return order.team == team && order.key == key && order.ready;
	});

	if (found == world.productionOrders.end())
	{
		Log::Warning("Nothing of " + key + " is ready for " + team + " to place");
		return;
	}

	const String command = "command:build,name:" + key + ",type:" + found->type + ",team:" + team +
						   ",x:" + StringField(entry, "x") + ",y:" + StringField(entry, "y");

	world.productionOrders.erase(found);

	RunAction(UIAction::AddGameItem, command);
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
	// A networked match drains inside the tick that raised the events, so the
	// two clients work through them at the same point in the same tick rather
	// than wherever their frames happened to fall.
	if (MultiplayerSetup::active && Net::Session::GetInstance().IsPlaying())
	{
		return;
	}

	DrainEvents();
}

void Renderer::DrainEvents()
{
	WorldState &world = WorldState::GetInstance();

	auto events = std::move(world.gameEvents);
	world.gameEvents.clear();

	const bool networked = MultiplayerSetup::active && Net::Session::GetInstance().IsPlaying();

	for (auto &gameEvent : events)
	{
		UIAction action = gameEvent.first;
		String value = gameEvent.second;

		// Only what a player asked for travels. It was raised on one machine,
		// so the others have no way to know of it. Everything else on this
		// queue a match works out for itself -- a unit dying, a prospector
		// finishing its extractor -- and every client works out the same thing
		// on the same tick. Sending those too would have each client raise its
		// own copy and every client apply all of them.
		if (networked && (action == UIAction::PlayerProduce || action == UIAction::PlayerPlace))
		{
			Log::Info("NET send " + UIActionToString(action) + ": " + value);

			Net::Session::GetInstance().SendCommand(
				{},
				{{"kind", "event"}, {"action", static_cast<int>(action)}, {"value", value}});

			continue;
		}

		RunAction(action, value);
	}
}

void Renderer::RunAction(UIAction action, const String &value)
{
	const auto found = functions.find(action);

	if (found != functions.end())
	{
		found->second(Renderer::GetInstance(), value);
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
