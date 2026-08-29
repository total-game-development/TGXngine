#include "Intro.h"
#include <fstream>
#include "Audio/Music.h"
#include "Config.h"
#include "UI/GraphicalButton.h"
#include "UI/TextButton.h"
#include "Window.h"
#include "WorldState.h"

namespace TGX
{
Intro::Intro()
{
	Log::Success("Intro Created");
}

Intro::~Intro()
{
	Log::Success("Deleted Intro");
}

void Intro::Init()
{
	String intro_path = "Resources/intro.json";

	if (!std::filesystem::exists(intro_path))
	{
		Log::Error("Intro file missing: " + intro_path);
		return;
	}

	json intro;
	std::ifstream intro_stream(intro_path);

	if (!(intro_stream >> intro))
	{
		Log::Error("Failed to parse intro JSON: " + intro_path);
		return;
	}

	sf::Image image;

	WorldState &world = WorldState::GetInstance();
	if (!(image.loadFromFile("Resources/images" + world.GetExtendedPath() + "tgx_splashscreen.png")))
	{
		Log::Error("Cannot load image\n");
	}

	backgroundTexture = sf::Texture();
	backgroundTexture.loadFromImage(image);

	background = sf::Sprite();
	background.setTexture(backgroundTexture);

	if (Config::Audio)
	{
		music = std::make_unique<Music>(Music::Song::Intro);
		music->Play();
	}

	json json_buttons = intro["intro"]["buttons"];
	buttons.reserve(json_buttons.size());

	for (std::size_t i = 0; i < json_buttons.size(); i++)
	{
		if (json_buttons[i].contains("text"))
		{
			buttons.emplace_back(std::make_shared<TextButton>());

			if (json_buttons[i].contains("text"))
			{
				std::dynamic_pointer_cast<TextButton>(buttons[i])->SetText(json_buttons[i]["text"]);
			}

			if (json_buttons[i].contains("font"))
			{
				std::dynamic_pointer_cast<TextButton>(buttons[i])->SetFont(json_buttons[i]["font"]);
			}

			if (json_buttons[i].contains("size"))
			{
				std::dynamic_pointer_cast<TextButton>(buttons[i])->SetSize(json_buttons[i]["size"]);
			}

			std::dynamic_pointer_cast<TextButton>(buttons[i])->CreateTextFontSize();

			if (json_buttons[i].contains("hover"))
			{
				std::dynamic_pointer_cast<TextButton>(buttons[i])->SetHover(json_buttons[i]["hover"]);
			}

			float x = json_buttons[i]["x"];
			float y = json_buttons[i]["y"];

			// Update x and y position for production
			if (world.IsProduction())
			{
				x += world.GetCanvasOffsetWidth() / 2.0f;
				y += world.GetCanvasOffsetHeight() / 2.0f;
			}

			buttons[i]->SetButton(
				json_buttons[i]["text"],
				UIActionFromString(json_buttons[i]["click"]["action"]),
				json_buttons[i]["click"]["value"],
				json_buttons[i]["alignment"],
				x,
				y,
				json_buttons[i]["width"],
				json_buttons[i]["height"],
				world.GetCanvasOffsetWidth(),
				world.GetCanvasOffsetHeight());

			if (json_buttons[i].contains("colour"))
			{
				std::dynamic_pointer_cast<TextButton>(buttons[i])->SetColour(json_buttons[i]["colour"]);
			}

			if (json_buttons[i].contains("font-style"))
			{
				std::dynamic_pointer_cast<TextButton>(buttons[i])->SetFontStyle(json_buttons[i]["font-style"].get_ref<String &>());
			}

			if (json_buttons[i].contains("outline-color"))
			{
				std::dynamic_pointer_cast<TextButton>(buttons[i])->SetOutlineColour(json_buttons[i]["outline-color"]);
			}

			if (json_buttons[i].contains("outline-size"))
			{
				std::dynamic_pointer_cast<TextButton>(buttons[i])->SetOutlineSize(json_buttons[i]["outline-size"]);
			}
		}
		else if (json_buttons[i].contains("image"))
		{
			buttons.emplace_back(std::make_shared<GraphicalButton>());

			buttons[i]->SetButton(
				json_buttons[i]["image"],
				UIActionFromString(json_buttons[i]["click"]["action"]),
				json_buttons[i]["click"]["value"],
				json_buttons[i]["alignment"],
				json_buttons[i]["x"],
				json_buttons[i]["y"],
				json_buttons[i]["width"],
				json_buttons[i]["height"],
				world.GetCanvasOffsetWidth(),
				world.GetCanvasOffsetHeight());
		}
	}

	Log::Success("Intro Init");
}

void Intro::Update()
{
}

void Intro::Draw()
{
	Window &window = Window::GetInstance();

	window.Draw(background);

	for (const auto &button : buttons)
	{
		button->Draw();
	}
}

void Intro::Click()
{
	// A copy: a button that loads a scene frees this one on the way, and Free
	// clears the vector the loop is walking.
	const Vector<Ref<Button>> pressed = buttons;

	for (const auto &button : pressed)
	{
		if (button->Click())
		{
			break;
		}
	}
}

void Intro::RightClick()
{
}

void Intro::Release()
{
}

void Intro::Close()
{
	Log::Success("Close Intro");

	if (Config::Audio && music)
	{
		music->Stop();
	}
}

void Intro::Free()
{
	buttons.clear();
}
} // namespace TGX
