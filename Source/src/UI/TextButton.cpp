#include "TextButton.h"
#include "Renderer.h"
#include "Window.h"

namespace TGX
{
TextButton::TextButton()
{
	this->size = 12;

	Log::Success("Text Button Created");
}

TextButton::~TextButton()
{
	Log::Success("Deleted Text Button");
}

bool TextButton::Click()
{
	WorldState &world = WorldState::GetInstance();
	// Same rectangle Draw hit-tests. It was passed x + width, which Intersect
	// adds to the corner again, so a button reached twice its width and the
	// whole screen below it -- invisible with one button, fatal with two.
	if (Intersect(world.GetMouseX(), world.GetMouseY(), x, y, static_cast<float>(width), static_cast<float>(height)))
	{
		if (action == UIAction::None)
		{
			return false;
		}

		Log::Info("Text Button - Action:" + UIActionToString(action) + ", Value: " + value);

		Renderer::GetInstance().functions[action](Renderer::GetInstance(), value);
		return true;
	}

	return false;
}

void TextButton::Draw()
{
	unsigned int red = (this->textColour & 0xFF0000) >> 16;
	unsigned int green = (this->textColour & 0x00FF00) >> 8;
	unsigned int blue = (this->textColour & 0x0000FF) >> 0;

	this->drawText.setFillColor(sf::Color(static_cast<sf::Uint8>(red), static_cast<sf::Uint8>(green), static_cast<sf::Uint8>(blue), 255));

	WorldState &world = WorldState::GetInstance();
	if (Intersect(world.GetMouseX(), world.GetMouseY(), x, y, static_cast<float>(width), static_cast<float>(height)))
	{
		if (isHover)
		{
			red = (hoverColour & 0xFF0000) >> 16;
			green = (hoverColour & 0x00FF00) >> 8;
			blue = (hoverColour & 0x0000FF) >> 0;

			this->drawText.setFillColor(sf::Color(static_cast<sf::Uint8>(red), static_cast<sf::Uint8>(green), static_cast<sf::Uint8>(blue), 255));
		}
	}

	Window &window = Window::GetInstance();
	window.Draw(this->drawText);
}

void TextButton::SetButton(const String & /*newText*/, UIAction newAction, const String &newValue, float newX, float newY, int newWidth, int newHeight)
{
	this->action = newAction;
	this->value = newValue;
	this->x = newX;
	this->y = newY;
	this->width = newWidth;
	this->height = newHeight;

	this->drawText.setPosition(this->x, this->y);
}

void TextButton::SetButton(
	const String &, UIAction newAction, const String &newValue, const String &alignment,
	float newX, float newY, int newWidth, int newHeight, int xOffset, int yOffset)
{
	this->action = newAction;
	this->value = newValue;

	this->x = newX;
	this->y = newY;
	this->width = newWidth;
	this->height = newHeight;

	Alignment alignmentValue = AlignmentFromString(alignment);

	if (alignmentValue == Alignment::Horizontal || alignmentValue == Alignment::Both)
	{
		this->x += static_cast<float>(xOffset);
	}

	if (alignmentValue == Alignment::Vertical || alignmentValue == Alignment::Both)
	{
		this->y += static_cast<float>(yOffset);
	}

	this->drawText.setPosition(this->x, this->y);
}
void TextButton::SetHover(const String &color)
{
	isHover = true;
	hoverColour = std::stoi(color, nullptr, 16);
}

void TextButton::CreateTextFontSize()
{
	this->drawText = sf::Text(text, font, size);
}

void TextButton::SetText(const String &newText)
{
	this->text = newText;
}

void TextButton::SetFont(const String &font_file)
{
	if (!font.loadFromFile("Resources/" + font_file + ".ttf"))
	{
		font.loadFromFile("Resources/courier.ttf");
		Log::Error("Font Error: " + font_file);
	}
	else
	{
		Log::Success("Font Success: " + font_file);
	}
}

void TextButton::SetFontStyle(String &font_style)
{
	int style = 0;

	if (font_style.find("bold") != std::string::npos)
	{
		style = style | sf::Text::Bold;
	}

	if (font_style.find("italic") != std::string::npos)
	{
		style = style | sf::Text::Italic;
	}

	if (font_style.find("underlined") != std::string::npos)
	{
		style = style | sf::Text::Underlined;
	}

	this->drawText.setStyle(style);
}

void TextButton::SetSize(int newSize)
{
	this->size = newSize;
}

void TextButton::SetOutlineSize(unsigned int newSize)
{
	this->outline_size = static_cast<int>(newSize);
	this->drawText.setOutlineThickness(static_cast<float>(outline_size));
}

void TextButton::SetColour(const String &colour)
{
	Log::Print("SetColour");
	Log::Print(colour);
	this->textColour = std::stoi(colour, nullptr, 16);

	unsigned int red = (this->textColour & 0xFF0000) >> 16;
	unsigned int green = (this->textColour & 0x00FF00) >> 8;
	unsigned int blue = (this->textColour & 0x0000FF) >> 0;

	this->drawText.setFillColor(sf::Color(static_cast<sf::Uint8>(red), static_cast<sf::Uint8>(green), static_cast<sf::Uint8>(blue), 255));
}

void TextButton::SetOutlineColour(const String &colour)
{
	this->outlineColour = std::stoi(colour, nullptr, 16);

	unsigned int red = (this->outlineColour & 0xFF0000) >> 16;
	unsigned int green = (this->outlineColour & 0x00FF00) >> 8;
	unsigned int blue = (this->outlineColour & 0x0000FF) >> 0;

	this->drawText.setOutlineColor(sf::Color(static_cast<sf::Uint8>(red), static_cast<sf::Uint8>(green), static_cast<sf::Uint8>(blue), 192));
}

void TextButton::Clear()
{
}
} // namespace TGX
