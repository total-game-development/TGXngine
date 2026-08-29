#include "Game.h"
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Mouse.hpp>
#include <Debug.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include "Config.h"
#include "Controller.h"
#include "Globals.h"
#include "ImageLoader.h"
#include "Mouse.h"
#include "Navigation.h"
#include "Physics.h"
#include "Window.h"
#include "WorldState.h"

namespace TGX
{
Game::Game()
{
	Log::Success("Game Created");
}

Game::~Game() = default;

void Game::Init()
{
	auto modulesJson = LoadJsonFile("Resources/modules.json");
	auto gameJson = LoadJsonFile("Resources/game.json");
	auto mapsJson = LoadJsonFile("Resources/maps.json");

	if (!modulesJson || !mapsJson || !gameJson)
	{
		Log::Error("Critical engine initialization failed due to corrupted or missing resource files.");
		return;
	}

	auto &jsonFiles = *modulesJson;
	auto &mapsJsonRef = *mapsJson;
	auto &gameJsonRef = *gameJson;

	if (!gameJsonRef.contains("game") || !gameJsonRef["game"].is_object() ||
		!gameJsonRef["game"].contains("startLevel") || !gameJsonRef["game"]["startLevel"].is_number_integer())
	{
		Log::Error("game.json structure is invalid or 'startLevel' is missing/not an integer!");
		return;
	}

	currentLevel = gameJsonRef["game"]["startLevel"].get<int>();

	// A skirmish agreed in the lobby picks the map; startLevel is the fallback.
	if (SkirmishSetup::active && SkirmishSetup::level >= 0)
	{
		currentLevel = SkirmishSetup::level;
	}

	WorldState &world = WorldState::GetInstance();
	world.SetCurrentLevel(currentLevel);

	if (!mapsJsonRef.contains("singleplayer") || !mapsJsonRef["singleplayer"].is_array() ||
		static_cast<size_t>(currentLevel) >= mapsJsonRef["singleplayer"].size())
	{
		Log::Error("Level index " + std::to_string(currentLevel) + " out of bounds or missing in maps.json");
		return;
	}

	if (!jsonFiles.contains("singleplayer") || !jsonFiles["singleplayer"].is_array() ||
		static_cast<size_t>(currentLevel) >= jsonFiles["singleplayer"].size())
	{
		Log::Error("Level index " + std::to_string(currentLevel) + " out of bounds or missing in modules.json");
		return;
	}

	json &level = mapsJsonRef["singleplayer"][currentLevel];
	json &files = jsonFiles["singleplayer"][currentLevel];

	Log::Info("Current Level Number: " + std::to_string(currentLevel));

	// The lobby writes its choices onto the level's own team list, so everything
	// downstream reads one answer instead of two.
	if (SkirmishSetup::active && !SkirmishSetup::slots.empty())
	{
		json teams = json::array();

		for (const SkirmishSetup::Slot &slot : SkirmishSetup::slots)
		{
			if (slot.role == "none")
			{
				continue;
			}

			const String role = SkirmishSetup::spectator ? String{"ai"} : slot.role;

			teams.push_back({{"name", slot.team}, {"type", role}});
		}

		level["teams"] = teams;
	}

	if (level.contains("teams"))
	{
		for (const auto &teamEntry : level["teams"])
		{
			if (teamEntry["type"] == "player")
			{
				world.SetTeam(teamEntry["name"]);
				break;
			}
		}
	}

	// Nobody took a side, so the view borrows the first one: the renderer, the
	// economy and the fog of war all need a perspective.
	if (SkirmishSetup::active && SkirmishSetup::spectator && !SkirmishSetup::slots.empty())
	{
		world.SetTeam(SkirmishSetup::slots.front().team);
	}

	world.SetBackgroundOffsetX(level["backgroundOffsetX"]);
	world.SetBackgroundOffsetY(level["backgroundOffsetY"]);
	world.SetBackgroundOffsetWidth(level["backgroundOffsetWidth"]);
	world.SetBackgroundOffsetHeight(level["backgroundOffsetHeight"]);
	world.SetMapGridWidth(level["mapGridWidth"]);
	world.SetMapGridHeight(level["mapGridHeight"]);

	Globals::mapGridWidth = level["mapGridWidth"];
	Globals::mapGridHeight = level["mapGridHeight"];

	Log::Print(Globals::mapGridWidth);
	Log::Print(Globals::mapGridHeight);

	json &requiredJsons = files["required_jsons"];
	if (!LoadExtraResources(currentLevel, level, requiredJsons))
	{
		return;
	}

	background = std::make_unique<Background>(level);

	Log::Print("Load essential assets");
	loader = std::make_unique<Loader>();

	Log::Print("Post Load essential assets");
	loader->AssignGameDLLs(files);
	if (WorldState::GetInstance().IsFogOfWarEnabled())
	{
		loader->AssignFogOfWar();
	}
	loader->AssignGameItems(level);
	loader->AssignGameResources(level);
	loader->AssignProjectiles(level);
	loader->AssignInterface(level);
	loader->AssignTriggers(level);

	gameItems = std::move(loader->GetGameItems());
	gameAssets = std::move(loader->GetGameAssets());
	gameProjectiles = std::move(loader->GetGameProjectiles());
	gameResources = std::move(loader->GetGameResources());
	gameInterfaces = std::move(loader->GetGameInterfaces());
	gameTriggers = std::move(loader->GetGameTriggers());

	if (level.contains("economy"))
	{
		loader->AssignEconomy(level);
		gameEconomies = std::move(loader->GetEconomies());
	}

	fogOfWarModule = std::move(loader->GetFogOfWar());

	font.loadFromFile("Resources/courier.ttf");
	this->fpsText = sf::Text("", font, 16);
	this->fpsText.setPosition(20, 100);
	this->drawVehiclesCollision = sf::Text("Vehicles Collision: ", font, 16);
	this->drawVehiclesCollision.setPosition(20, 160);

	Log::Success("Game Init");
}

void Game::Update()
{
	WorldState &world = WorldState::GetInstance();
	Physics &physics = Physics::GetInstance();

	physics.Update();

	HandlePanning();
	background->SyncPosition();

	world.SetItemUnderCursor(false);
	world.SetEnemyItemUnderCursor(false);
	world.SetResourceUnderCursor(false);

	for (const auto &gameItem : gameItems)
	{
		gameItem->Update();
	}

	for (std::size_t i = 0; i < gameItems.size(); i++)
	{
		gameItems[i]->ProcessOrders(world.items[i]);
	}

	for (const auto &gameAsset : gameAssets)
	{
		gameAsset->Update();
	}

	for (const auto &gameResource : gameResources)
	{
		gameResource->Update();
	}

	for (const auto &gameEconomy : gameEconomies)
	{
		gameEconomy->Update();
	}

	for (const auto &gameProjectile : gameProjectiles)
	{
		gameProjectile->Update();
	}

	for (const auto &gameInterface : gameInterfaces)
	{
		gameInterface->Update();
	}

	if (fogOfWarModule)
	{
		fogOfWarModule->Update();
	}

	HandleSingleSelection();

	world.SetLeftClicked(false);
	world.SetRightClicked(false);

	Navigation::DeleteAllMarkers(world.currentTerrainMapPassableGrid);
}

void Game::Draw()
{
	Window &window = Window::GetInstance();

	background->Draw();

	for (const auto &gameResource : gameResources)
	{
		gameResource->Draw();
	}

	for (const auto &gameItem : gameItems)
	{
		ItemInstance *instance = gameItem->GetItemInstance();
		if (instance && instance->GetTeam() != WorldState::GetInstance().GetTeam() && !instance->isVisible())
		{
			continue;
		}
		gameItem->Draw();
	}

	for (const auto &gameProjectile : gameProjectiles)
	{
		gameProjectile->Draw();
	}

	if (fogOfWarModule)
	{
		fogOfWarModule->Draw();
	}

	for (const auto &gameInterface : gameInterfaces)
	{
		gameInterface->Draw();
	}

	if (Debug::showGrid)
	{
		grid->Draw();
	}

	if ((frame % Globals::targetFPS) == 0)
	{
		WorldState &world = WorldState::GetInstance();

		if (world.IsDebugOnScreen())
		{
			fpsText.setString("FPS: " + std::to_string(floor(world.GetFPS())));
		}
	}

	window.Draw(fpsText);

	frame++;
}

void Game::Click()
{
	WorldState &world = WorldState::GetInstance();
	world.SetLeftClicked(true);

	world.SetSkipSelectionRemoval(false);

	for (const auto &gameInterface : gameInterfaces)
	{
		gameInterface->Click();
	}

	if (!world.SkipSelectionRemoval())
	{
		for (const auto &gameItem : gameItems)
		{
			gameItem->Clear();
		}
	}

	if (!world.SkipSelectionRemoval())
	{
		ClearSelection();
	}
}

void Game::RightClick()
{
	WorldState &world = WorldState::GetInstance();
	world.SetRightClicked(true);

	currentOrderId = (currentOrderId + 1) % 65536;

	for (std::size_t i = 0; i < gameItems.size(); i++)
	{
		for (std::size_t j = 0; j < world.selected.size(); j++)
		{
			if (world.selected[j] == world.items[i]->GetUid())
			{
				Mouse &mouse = Mouse::GetInstance();
				Orders *orders = mouse.CurrentOrder();

				world.items[i]->GetOrders()->order = orders->order;
				world.items[i]->GetOrders()->toX = orders->toX;
				world.items[i]->GetOrders()->toY = orders->toY;
				world.items[i]->GetOrders()->id = static_cast<int>(currentOrderId);

				gameItems[i]->SendOrders(world.items[i]);

				break;
			}
		}
	}
}

void Game::Release()
{
	Mouse &mouse = Mouse::GetInstance();

	if (!mouse.IsSelectGameItems())
	{
		return;
	}

	WorldState &world = WorldState::GetInstance();
	float selectionX = mouse.GetSelectionBoxX() - static_cast<float>(world.GetMapXOffset()) - Globals::mapOffsetX;
	float selectionY = mouse.GetSelectionBoxY() - static_cast<float>(world.GetMapYOffset()) - Globals::mapOffsetY;
	float selectionWidth = selectionX + mouse.GetSelectionBoxWidth();
	float selectionHeight = selectionY + mouse.GetSelectionBoxHeight();

	for (const auto &gameItem : gameItems)
	{
		auto *itemIinstance = gameItem->GetItemInstance();

		if (itemIinstance->GetTeam() != world.GetTeam())
		{
			continue;
		}

		float itemX = itemIinstance->GetX() * Globals::grid_size;
		float itemY = itemIinstance->GetY() * Globals::grid_size;

		bool isOverlapping =
			selectionX <= itemX &&
			selectionWidth >= itemX &&
			selectionY <= itemY &&
			selectionHeight >= itemY;

		if (isOverlapping)
		{
			itemIinstance->SetSelected(true);
			world.selected.emplace_back(itemIinstance->GetUid());
		}
	}
}

void Game::HandlePanning()
{
	WorldState &world = WorldState::GetInstance();
	Mouse &mouse = Mouse::GetInstance();

	float frameDistance = PANNING_SPEED * world.GetDeltaTime();

	float cursorX = sf::Joystick::getAxisPosition(0, sf::Joystick::X);
	float cursorY = sf::Joystick::getAxisPosition(0, sf::Joystick::Y);

	float camX = sf::Joystick::getAxisPosition(0, sf::Joystick::Z);
	float camY = sf::Joystick::getAxisPosition(0, sf::Joystick::R);

	auto normalize = [](float v) {
		if (std::abs(v) < 15.f)
		{
			return 0.f;
		}
		return v / 100.f;
	};

	cursorX = normalize(cursorX);
	cursorY = normalize(cursorY);

	camX = normalize(camX);
	camY = normalize(camY);

	if (cursorX != 0.f || cursorY != 0.f)
	{
		float speed = 1000.f * world.GetDeltaTime();

		float newX = mouse.x + cursorX * speed;
		float newY = mouse.y + cursorY * speed;

		newX = std::clamp(newX, 0.f, (float)Globals::canvasWidth + world.GetCanvasOffsetWidth());
		newY = std::clamp(newY, 0.f, (float)Globals::canvasHeight + world.GetCanvasOffsetHeight());

		mouse.Moved(newX, newY);
	}

	if (camX != 0.f)
	{
		float move = camX * frameDistance;

		float maxScroll =
			static_cast<float>(background->GetWidth()) -
			(Globals::canvasWidth - world.GetBackgroundOffsetWidth() + world.GetCanvasOffsetWidth());

		float oldOffset = Globals::mapOffsetX;

		Globals::mapOffsetX = std::clamp(oldOffset + move, 0.0f, maxScroll);

		float actualMove = Globals::mapOffsetX - oldOffset;

		world.SetPanX(Globals::mapOffsetX);
		world.UpdateMapXOffset(-actualMove);
	}

	if (camY != 0.f)
	{
		float move = camY * frameDistance;

		float maxScrollY =
			static_cast<float>(background->GetHeight()) -
			(Globals::canvasHeight - world.GetBackgroundOffsetY() + world.GetCanvasOffsetHeight());

		float oldOffset = Globals::mapOffsetY;

		Globals::mapOffsetY = std::clamp(oldOffset + move, 0.0f, maxScrollY);

		float actualMove = Globals::mapOffsetY - oldOffset;

		world.SetPanY(Globals::mapOffsetY);
		world.UpdateMapYOffset(-actualMove);
	}

	if (mouse.x < 0 || mouse.x > static_cast<float>(Globals::canvasWidth + world.GetCanvasOffsetWidth()))
	{
		return;
	}
	if (mouse.y < 0 || mouse.y > static_cast<float>(Globals::canvasHeight + world.GetCanvasOffsetHeight()))
	{
		return;
	}

	if (mouse.x <= PANNING_THRESHOLD && Globals::mapOffsetX > 0 && camX == 0.f)
	{
		float move = std::min(frameDistance, Globals::mapOffsetX);
		Globals::mapOffsetX -= move;

		if (Globals::mapOffsetX < 0.001f)
		{
			Globals::mapOffsetX = 0.0f;
		}

		world.SetPanX(Globals::mapOffsetX);
		world.UpdateMapXOffset(move);
	}

	if (mouse.x >= static_cast<float>(Globals::canvasWidth + world.GetCanvasOffsetWidth()) - PANNING_THRESHOLD && camX == 0.f)
	{
		float maxScroll =
			static_cast<float>(background->GetWidth()) -
			(Globals::canvasWidth - world.GetBackgroundOffsetWidth() + world.GetCanvasOffsetWidth());

		if (Globals::mapOffsetX < maxScroll)
		{
			float move = std::min(frameDistance, maxScroll - Globals::mapOffsetX);
			Globals::mapOffsetX += move;

			if (Globals::mapOffsetX > maxScroll - 0.001f)
			{
				Globals::mapOffsetX = maxScroll;
			}

			world.SetPanX(Globals::mapOffsetX);
			world.UpdateMapXOffset(-move);
		}
	}

	if (mouse.y <= PANNING_THRESHOLD && Globals::mapOffsetY > 0 && camY == 0.f)
	{
		float move = std::min(frameDistance, Globals::mapOffsetY);
		Globals::mapOffsetY -= move;

		if (Globals::mapOffsetY < 0.001f)
		{
			Globals::mapOffsetY = 0.0f;
		}

		world.SetPanY(Globals::mapOffsetY);
		world.UpdateMapYOffset(move);
	}

	if (mouse.y >= static_cast<float>(Globals::canvasHeight + world.GetCanvasOffsetHeight()) - PANNING_THRESHOLD && camY == 0.f)
	{
		float maxScrollY =
			static_cast<float>(background->GetHeight()) -
			(Globals::canvasHeight - world.GetBackgroundOffsetY() + world.GetCanvasOffsetHeight());

		if (Globals::mapOffsetY < maxScrollY)
		{
			float move = std::min(frameDistance, maxScrollY - Globals::mapOffsetY);
			Globals::mapOffsetY += move;

			if (Globals::mapOffsetY > maxScrollY - 0.001f)
			{
				Globals::mapOffsetY = maxScrollY;
			}

			world.SetPanY(Globals::mapOffsetY);
			world.UpdateMapYOffset(-move);
		}
	}
}

void Game::HandleSingleSelection()
{
	WorldState &world = WorldState::GetInstance();

	if (world.IsLeftClicked() && !world.selected.empty())
	{
		double minDistance = std::numeric_limits<double>::infinity();
		int bestUid = 0;
		int bestIndex = -1;

		Vector<int> localSelection = world.selected;

		for (int uid : localSelection)
		{
			int currentIndex = TGX::LookUp::Get(uid);
			if (currentIndex == -1)
			{
				continue;
			}

			auto &selectedItem = world.items[currentIndex];

			if (selectedItem->GetHidden())
			{
				continue;
			}

			float centerX = (selectedItem->GetX() * Globals::grid_size);
			float centerY = (selectedItem->GetY() * Globals::grid_size);

			float dx = (world.GetMouseX() - world.GetMapXOffset()) - centerX;
			float dy = (world.GetMouseY() - world.GetMapYOffset()) - centerY;

			double distanceSq = (dx * dx) + (dy * dy);

			if (distanceSq < minDistance)
			{
				minDistance = distanceSq;
				bestUid = uid;
				bestIndex = currentIndex;
			}
		}

		world.selected.clear();

		if (bestIndex != -1)
		{
			world.selected.push_back(bestUid);
			world.items[bestIndex]->SetSelected(true);
		}
	}
}

void Game::ClearSelection()
{
	WorldState &world = WorldState::GetInstance();
	while (world.selected.size() > 0)
	{
		for (const auto &gameItem : gameItems)
		{
			gameItem->GetItemInstance()->SetSelected(false);
		}

		world.selected.pop_back();
	}
}

void Game::Close()
{
	Log::Success("Close Game");

	// Consumed by the match it set up. Left standing it would hijack the next
	// launch, which reads startLevel.
	SkirmishSetup::Clear();

	background.reset();
	item.reset();
	grid.reset();
	wayPoints.reset();

	for (auto &gameItem : gameItems)
	{
		gameItem->Delete();
	}
	gameItems.clear();

	for (auto &gameAsset : gameAssets)
	{
		gameAsset->Clear();
	}
	gameAssets.clear();

	for (auto &res : gameResources)
	{
		res.reset();
	}
	gameResources.clear();

	for (auto &proj : gameProjectiles)
	{
		proj.reset();
	}
	gameProjectiles.clear();

	for (auto &eco : gameEconomies)
	{
		eco.reset();
	}
	gameEconomies.clear();

	for (auto &ui : gameInterfaces)
	{
		ui->Clear();
		ui.reset();
	}
	gameInterfaces.clear();

	for (auto &trigger : gameTriggers)
	{
		trigger->Clear();
		trigger->Delete();
		trigger.reset();
	}
	gameTriggers.clear();

	fogOfWarModule.reset();

	ImageLoader::Delete();

	WorldState &world = WorldState::GetInstance();
	world.Clear();
	world.RestMapOffset();

	Globals::mapOffsetX = 0.0f;
	Globals::mapOffsetY = 0.0f;

	Physics &physics = Physics::GetInstance();
	physics.Clear();

	loader->ResetUIDCounter();
}

void Game::Free()
{
	Log::Success("Free Game");

	WorldState::GetInstance().Clear();

	const String filePath = "Resources/modules.json";
	const String mapsPath = "Resources/maps.json";

	if (!std::filesystem::exists(filePath) || !std::filesystem::exists(mapsPath))
	{
		Log::Error("Files missing during Free");
		return;
	}

	json jsonFiles;
	{
		std::ifstream fileStream(filePath);
		fileStream >> jsonFiles;
	}

	json mapsJson;
	{
		std::ifstream mapsStream(mapsPath);
		mapsStream >> mapsJson;
	}

	if (jsonFiles.contains("singleplayer") &&
		mapsJson.contains("singleplayer"))
	{
		json &level = mapsJson["singleplayer"][currentLevel];
		json &files = jsonFiles["singleplayer"][currentLevel];

		if (loader)
		{
			loader->ReleaseGameDLLs(level, files);
		}
	}

	Log::Clean("Game Resources Freed");
}

void Game::AddGameItem(json &jsonItem)
{
	Log::Info("Add Game Item");

	WorldState &world = WorldState::GetInstance();

	static int uid = 0;
	jsonItem["uid"] = uid;

	size_t currentSize = loader->GetGameItems().size();

	loader->AddGameItem(uid, jsonItem);

	if (loader->GetGameItems().size() == currentSize)
	{
		Log::Error("Loader failed to add item.");
		return;
	}

	gameItems.emplace_back(std::move(loader->GetGameItems().back()));
	loader->GetGameItems().pop_back();

	auto byPriorityGame = [](const auto &a, const auto &b) {
		return a->GetItemInstance()->GetPriority() < b->GetItemInstance()->GetPriority();
	};

	std::ranges::sort(gameItems, byPriorityGame);

	auto byPriorityWorld = [](const auto &a, const auto &b) {
		return a->GetPriority() < b->GetPriority();
	};

	std::ranges::sort(world.items, byPriorityWorld);

	uid++;
}

void Game::RemoveGameItem(json &jsonItem)
{
	WorldState &world = WorldState::GetInstance();
	int targetUid = jsonItem["uid"];

	Log::Success("Removing Item with UID: " + std::to_string(targetUid));

	std::erase_if(gameItems, [targetUid](const auto &item) {
		return item->GetItemInstance()->GetUid() == targetUid;
	});

	std::erase_if(world.items, [targetUid](const auto &instance) {
		return instance->GetUid() == targetUid;
	});
}

bool Game::LoadExtraResources(int inCurrentLevel, json &level, json &requiredJsons)
{
	for (auto &requiredJson : requiredJsons)
	{
		if (ResourceTypeFromString(requiredJson["type"]) == ResourceType::Obstruction)
		{
			std::string filename;

			if (requiredJson.contains("name") && requiredJson["name"].is_string())
			{
				filename = requiredJson["name"].get<std::string>();
			}
			else
			{
				Log::Error("Key 'name' is missing or is not a string type inside required_jsons element!");
				return false;
			}

			String obstruction_path;
			if (filename.rfind(".json") != std::string::npos)
			{
				obstruction_path = "Resources/" + filename;
			}
			else
			{
				obstruction_path = "Resources/" + filename + ".json";
			}

			Log::Warning(obstruction_path);

			if (!std::filesystem::exists(obstruction_path))
			{
				Log::Error("Obstruction file missing: " + obstruction_path);
				return false;
			}

			json obstruction_json;
			std::ifstream obstruction_stream(obstruction_path);

			if (!(obstruction_stream >> obstruction_json))
			{
				Log::Error("Failed to parse obstruction JSON: " + obstruction_path);
				return false;
			}

			grid = std::make_unique<Grid>(level, obstruction_json);
			grid->BuildPassableGrid();

			wayPoints = std::make_unique<WayPoints>();
		}
	}
	return true;
}

std::optional<nlohmann::json> Game::LoadJsonFile(std::string_view filePath)
{
	namespace fs = std::filesystem;

	if (!fs::exists(filePath))
	{
		Log::Error("File missing: " + String(filePath));
		return std::nullopt;
	}

	std::ifstream file(fs::path{filePath}, std::ios::binary | std::ios::ate);
	if (!file.is_open())
	{
		return std::nullopt;
	}

	auto size = file.tellg();
	String buffer(static_cast<size_t>(size), '\0');
	file.seekg(0);
	file.read(buffer.data(), size);

	auto parsedJson = nlohmann::json::parse(buffer, nullptr, false, true);

	if (parsedJson.is_discarded())
	{
		Log::Error("Failed to parse JSON file (malformed syntax): " + std::string(filePath));
		return std::nullopt;
	}

	return parsedJson;
}
} // namespace TGX
