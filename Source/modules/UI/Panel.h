#pragma once

#include <SFML/Graphics.hpp>
#include "Page.h"
#include "Widget.h"

namespace TGX::UI
{
class Panel : public Widget
{
private:
	Page page;

	String windowPath;
	String imagePath;

	sf::Sprite background;
	sf::Sprite closeButton;
	sf::Sprite image;
	sf::Sprite header;

	sf::Texture *backgroundTexture = nullptr;
	sf::Texture *closeTexture = nullptr;
	sf::Texture *imageTexture = nullptr;
	sf::Texture *headerTexture = nullptr;

	Vector<String> body;

	sf::Vector2f grab;

	bool dragging = false;
	bool dismissed = false;
	bool arranged = false;

	int opened = -1;
	int cascade = 0;

	sf::FloatRect Close() const;
	sf::FloatRect Back() const;
	sf::FloatRect Entry(std::size_t index) const;

	float BodyTop() const;
	void Compose();
	void DrawInbox();
	void DrawMail();
	void DrawBody();

public:
	Panel(Element inElement, Page inPage, String inWindowPath, String inImagePath);

	void SetCascade(int step);

	sf::FloatRect Content() const;

	void Arrange(sf::Vector2f view) override;
	void Draw() override;
	bool Press(sf::Vector2f point) override;
	void Release() override;
	void Move(sf::Vector2f point) override;
	bool Key(int code) override;
	bool Dismissed() const override;
	String Identity() const override;

	void Dismiss();
};
} // namespace TGX::UI
