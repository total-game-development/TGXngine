#include "Sidebar.h"
#include <fstream>
#include "StringUtils.hpp"
#include "Window.h"
#include "WorldState.h"

namespace TGX
{
Sidebar::Sidebar() = default;

Sidebar::~Sidebar()
{
	Log::Print("Delete Sidebar");
}

void Sidebar::Load(const String &name)
{
	String sidebarPath = "Resources/" + name + ".json";

	if (!std::filesystem::exists(sidebarPath))
	{
		Log::Error("Sidebar configuration file missing: " + sidebarPath);
		return;
	}

	json sidebarJson;
	std::ifstream sidebarStream(sidebarPath);

	if (!(sidebarStream >> sidebarJson))
	{
		Log::Error("Failed to parse sidebar JSON: " + sidebarPath);
		return;
	}

	String sidebar_fg = sidebarJson["sidebar"]["foreground"];

	Log::Print(sidebar_fg);
	WorldState &world = WorldState::GetInstance();

	sf::Image image;
	if (!(image.loadFromFile("Resources/images" + world.GetExtendedPath() + sidebar_fg)))
	{
		Log::Print("Cannot load image");
	}

	sidebarTexture = sf::Texture();
	sidebarTexture.loadFromImage(image);

	sidebar = sf::Sprite();
	sidebar.setTexture(sidebarTexture);

	json json_buttons = sidebarJson["sidebar"]["buttons"];

	buttons.reserve(json_buttons.size());

	for (auto &json_button : json_buttons)
	{
		buttons.emplace_back();

		SidebarButton &sidebarButton = buttons.back();

		int frame = json_button["frame"];

		sidebarButton.SetButton(
			"Resources/" + std::string(json_button["image"]) + std::to_string(frame) + std::string(json_button["extension"]),
			UIActionFromString(json_button["click"]["action"]),
			json_button["click"]["value"],
			json_button["click"]["type"],
			json_button["attach"],
			json_button["wait"],
			json_button["alignment"],
			json_button["x"],
			json_button["y"], 0, 0,
			world.GetCanvasOffsetWidth(),
			world.GetCanvasOffsetHeight());

		float durationInSeconds = static_cast<float>(json_button["duration"]) / static_cast<float>(world.GetTargetFPS());
		sidebarButton.SetDuration(durationInSeconds);

		sidebarButton.SetFrame(json_button["frame"]);
		sidebarButton.SetFrames(json_button["frames"]);
		sidebarButton.SetCost(json_button["cost"]);
		sidebarButton.SetPowerUsage(json_button["powerusage"]);

		if (json_button.contains("buildableGrid"))
		{
			sidebarButton.AddBuildableGrid(json_button["buildableGrid"]);
		}
		else
		{
			Log::Print(StringConcat("A buildable grid was not present for ", std::string(json_button["image"])));
		}

		sidebarButton.AssignAdditionalButton(json_button["image"], json_button["extension"]);
	}
}

void Sidebar::Draw()
{
	Window &window = Window::GetInstance();

	for (auto &button : buttons)
	{
		button.DrawPlacement();
	}

	window.Draw(sidebar);

	for (auto &button : buttons)
	{
		button.Draw();
	}
}

void Sidebar::Update()
{
	for (auto &button : buttons)
	{
		button.Update();
	}
}

void Sidebar::Click()
{
	Reset();

	WorldState &world = WorldState::GetInstance();

	if (world.selected.size() > 0)
	{
		for (auto &button : buttons)
		{
			if (button.Click())
			{
				if (button.buttonState == SidebarButton::States::Wait)
				{
					Log::Print("Click the sidebar button");

					world.SetSkipSelectionRemoval(true);
					world.SetPlacement(true);
					world.SetBuilt(false);
					world.pendingQueue = button.GetValue() + "," + button.GetType();
					button.buttonState = SidebarButton::States::Placement;
					button.drawState = SidebarButton::States::Placement;
				}
				else if (button.buttonState == SidebarButton::States::Ready || button.buttonState == SidebarButton::States::Off)
				{
					world.SetSkipSelectionRemoval(true);

					button.buttonState = SidebarButton::States::On;
					button.drawState = SidebarButton::States::On;
				}
				else if (button.buttonState == SidebarButton::States::On)
				{
					world.SetSkipSelectionRemoval(true);

					if (button.GetPowerUsage() < 0)
					{
						Log::Debug(world.GetPowerTotal());
						Log::Debug(-button.GetPowerUsage());
						Log::Debug(world.GetPowerTotal() + -button.GetPowerUsage());
						Log::Debug(world.GetPowerUsage());
						Log::Debug(world.GetPowerTotal());
					}
					else if (button.GetPowerUsage() > 0)
					{
						if (world.GetPowerUsage() + button.GetPowerUsage() < world.GetPowerTotal())
						{
							Log::Debug(world.GetPowerUsage());
							Log::Debug(world.GetPowerTotal());
						}
						else
						{
							Log::Debug("Insufficient power");
							button.buttonState = SidebarButton::States::Off;
							button.drawState = SidebarButton::States::Off;
						}
					}

					if (!button.HasFreeDeployBerth())
					{
						Log::Warning("Deploy berth occupied, move the last unit clear before building another");
						button.buttonState = SidebarButton::States::Off;
						button.drawState = SidebarButton::States::Off;
					}
					else if (world.GetCash() - button.GetCost() >= 0)
					{
						Log::Success("Buy!");

						world.SetCash(world.GetCash() - button.GetCost());

						button.buttonState = SidebarButton::States::Progress;
						button.drawState = SidebarButton::States::Progress;
					}
					else
					{
						Log::Debug("Insufficient  cash");
						button.buttonState = SidebarButton::States::Off;
						button.drawState = SidebarButton::States::Off;
					}
				}

				break;
			}
		}
	}
}

void Sidebar::Reset()
{
	for (auto &button : buttons)
	{
		button.ResetDrawState();
	}
}

void Sidebar::Restore()
{
	for (auto &button : buttons)
	{
		button.ResetButtonState();
	}
}

void Sidebar::Clear()
{
	for (auto &button : buttons)
	{
		button.Clear();
	}

	buttons.clear();
}
} // namespace TGX
