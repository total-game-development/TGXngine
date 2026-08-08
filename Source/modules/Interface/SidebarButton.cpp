#include "SidebarButton.h"

#include <utility>
#include "Lookup.h"
#include "StringUtils.hpp"
#include "Window.h"
#include "WorldState.h"

namespace TGX
{
SidebarButton::SidebarButton() = default;

SidebarButton::~SidebarButton() = default;

bool SidebarButton::Click()
{
	WorldState &world = WorldState::GetInstance();
	if (Intersect(world.GetMouseX(), world.GetMouseY(), x, y, static_cast<float>(width), static_cast<float>(height)))
	{
		if (action == UIAction::None || value == "")
		{
			return false;
		}

		world.gameEvents.emplace_back(action, value);

		return true;
	}

	return false;
}

void SidebarButton::Draw()
{
	Window &window = Window::GetInstance();
	window.Draw(sprites[frame]);
}

void SidebarButton::DrawPlacement()
{
	if (drawState != States::Placement)
	{
		return;
	}

	WorldState &world = WorldState::GetInstance();

	world.SetPlacement(true);
	world.SetBuilt(true);

	constexpr int CELL_SIZE = 20;

	const int gridX = static_cast<int>(world.GetGameX() / CELL_SIZE);
	const int gridY = static_cast<int>(world.GetGameY() / CELL_SIZE);

	const int cellsWide = static_cast<int>(buildableCells.getSize().x) / CELL_SIZE;
	const int cellsHigh = static_cast<int>(buildableCells.getSize().y) / CELL_SIZE;

	const int startY = gridY - 4;
	const int endX = gridX + cellsWide - 1;
	const int endY = startY + cellsHigh - 1;

	bool canBuild = true;

	if (gridX < 0 ||
		startY < 0 ||
		endX >= world.GetMapGridWidth() ||
		endY >= world.GetMapGridHeight())
	{
		canBuild = false;
	}
	else
	{
		for (int placementY = 0; placementY < cellsHigh && canBuild; ++placementY)
		{
			for (int placementX = 0; placementX < cellsWide; ++placementX)
			{
				if (world.currentTerrainMapPassableGrid[startY + placementY][gridX + placementX] >= 1)
				{
					canBuild = false;
					break;
				}
			}
		}
	}

	buildableCells.setFillColor(
		canBuild ? sf::Color(0, 255, 0, 76) : sf::Color(255, 0, 0, 76));

	world.SetBuilt(canBuild);

	buildableCells.setPosition(
		std::round((gridX * CELL_SIZE) - world.GetPanX()),
		std::round((gridY * CELL_SIZE) - world.GetPanY()));

	Window &window = Window::GetInstance();
	window.Draw(buildableCells);
}

void SidebarButton::Update()
{
	WorldState &world = WorldState::GetInstance();
	float deltaTime = world.GetDeltaTime();

	if (buttonState == States::Progress)
	{
		buttonState = States::Progress;
		durationCounter += deltaTime;

		if (durationCounter >= duration)
		{
			if (this->waitForClick)
			{
				buttonState = States::Wait;
				drawState = States::Wait;
				durationCounter = 0.0f;

				return;
			}

			BuildImmediately();

			durationCounter = 0.0f;

			buttonState = States::Off;

			return;
		}
	}

	if (world.selected.size() > 0)
	{
		int index = LookUp::Get(world.selected[0]);
		if (index != -1)
		{
			if (world.items[index]->GetName() == attached)
			{
				if (buttonState == States::Off || buttonState == States::On)
				{
					buttonState = States::On;
					drawState = States::On;
				}
				else if (buttonState == States::Progress)
				{
					drawState = States::Progress;
				}
				else if (buttonState == States::Wait)
				{
					drawState = States::Wait;
				}
			}
		}
	}

	frame = 0;

	if (drawState == States::On || drawState == States::Placement)
	{
		frame = 1;
	}
	else if (drawState == States::Progress)
	{
		frame = 2;
	}
	else if (drawState == States::Wait)
	{
		frame = 3;
	}
}

void SidebarButton::SetButton(
	const String &text, UIAction newAction, String newValue, String newType, String newAttached, bool wait,
	const String &alignment, float newX, float newY, int /*newWidth*/, int /*newHeight*/, int xOffset, int yOffset)
{
	sf::Image image;
	if (!(image.loadFromFile(text)))
	{
		Log::Print("Cannot load sidebar image");
	}

	auto texture = std::make_unique<sf::Texture>();
	texture->loadFromImage(image);
	spriteTextures.push_back(std::move(texture));

	sprites.emplace_back();
	sprites.back().setTexture(*spriteTextures.back());

	this->action = newAction;
	this->value = std::move(newValue);
	this->type = std::move(newType);
	this->attached = std::move(newAttached);
	this->waitForClick = wait;

	this->durationCounter = 0.0f;

	this->x = newX;
	this->y = newY;

	Alignment alignmentValue = AlignmentFromString(alignment);

	if (alignmentValue == Alignment::Horizontal)
	{
		this->x = this->x + static_cast<float>(xOffset);
	}

	if (alignmentValue == Alignment::Vertical)
	{
		this->y = this->y + static_cast<float>(yOffset);
	}

	if (alignmentValue == Alignment::Both)
	{
		this->x = this->x + static_cast<float>(xOffset);
		this->y = this->y + static_cast<float>(yOffset);
	}

	sprites.back().move(this->x, this->y);
	this->width = static_cast<int>(spriteTextures.back()->getSize().x);
	this->height = static_cast<int>(spriteTextures.back()->getSize().y);
}

void SidebarButton::SetFrame(int inFrame)
{
	frame = inFrame;
}

void SidebarButton::SetFrames(int inFrames)
{
	frames = inFrames;
}

void SidebarButton::SetDuration(float inDuration)
{
	duration = inDuration;
}

void SidebarButton::SetCost(int inCost)
{
	cost = inCost;
}

void SidebarButton::SetPowerUsage(int inPowerUsage)
{
	powerUsage = inPowerUsage;
}

void SidebarButton::AddBuildableGrid(json buildableGrid)
{
	buildableCells = sf::RectangleShape(sf::Vector2f(
		static_cast<float>(buildableGrid["x"]) * 20,
		static_cast<float>(buildableGrid["y"]) * 20));
}

void SidebarButton::AssignAdditionalButton(const std::string &image_filename, const std::string &extension)
{
	for (int i = frame + 1; i < frames; i++)
	{
		sf::Image image;

		if (!(image.loadFromFile(StringConcat("Resources/", image_filename, i, extension))))
		{
			Log::Print("Cannot load image");
		}

		auto texture = std::make_unique<sf::Texture>();
		texture->loadFromImage(image);
		spriteTextures.push_back(std::move(texture));

		sprites.emplace_back();
		sprites.back().setTexture(*spriteTextures.back());

		sprites.back().move(x, y);
	}
}

UIAction SidebarButton::GetAction()
{
	return action;
}

String SidebarButton::GetValue()
{
	return value;
}

String SidebarButton::GetType()
{
	return type;
}

int SidebarButton::GetCost()
{
	return cost;
}

int SidebarButton::GetPowerUsage()
{
	return powerUsage;
}

bool SidebarButton::HasFreeDeployBerth() const
{
	// A yard fits one hull at a time. While its berth still holds the last hull
	// built, another cannot be laid down without stacking one on top of the
	// other. The player clears the berth by moving that hull out of it.
	//
	// Only ships work this way. The land producers cycle through many slots and
	// never reserve one, so gating them on a free slot would stop their
	// production outright once every slot had been used.
	if (type != "ships")
	{
		return true;
	}

	WorldState &world = WorldState::GetInstance();

	auto berths = world.deployMap.find(world.primaryItems[attached]);

	if (berths == world.deployMap.end())
	{
		return true;
	}

	for (const auto &berth : berths->second)
	{
		if (std::get<2>(berth) == INT_MIN)
		{
			return true;
		}
	}

	return false;
}

void SidebarButton::BuildImmediately()
{
	Log::Print("BuildImmediately");

	WorldState &world = WorldState::GetInstance();

	world.primaryItems[attached];

	Log::Print(std::to_string(world.primaryItems[attached]));

	Log::Print(world.pendingQueue);

	String builtCommand = StringConcat("command:", "build");
	builtCommand += ",";
	String builtName = StringConcat("name:", GetValue());
	builtCommand += builtName + ",";
	String builtType = StringConcat("type:", GetType());
	builtCommand += builtType + ",";
	String builtTeam = StringConcat("team:", world.GetTeam());
	builtCommand += builtTeam;

	world.gameEvents.emplace_back(UIAction::AddGameItem, builtCommand);
}

void SidebarButton::ResetButtonState()
{
	buttonState = States::Off;
}

void SidebarButton::ResetDrawState()
{
	drawState = States::Off;
}

void SidebarButton::Clear()
{
	sprites.clear();
	spriteTextures.clear();
	frame = 0;
}
} // namespace TGX
