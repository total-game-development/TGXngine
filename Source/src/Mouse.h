#pragma once

#include <SFML/Graphics.hpp>
#include "Core.h"
#include "Orders.h"

namespace TGX
{

class Mouse
{
public:
	float x = 0;
	float y = 0;

	void Suppress(bool inSuppressed);

	void Click();
	void RightClick();
	void Release();
	void Moved(float x, float y);
	void Update();
	void Draw();
	void DrawCursor(Vector<sf::Sprite> cursor);
	Orders *CurrentOrder();

	bool IsSelectGameItems() const;

	float GetSelectionBoxX() const;
	float GetSelectionBoxY() const;
	float GetSelectionBoxWidth() const;
	float GetSelectionBoxHeight() const;

	static Mouse &GetInstance();

	Mouse(const Mouse &) = delete;
	Mouse &operator=(const Mouse &) = delete;

	Mouse(Mouse &&) = delete;
	Mouse &operator=(Mouse &&) = delete;

public:
	bool dragPressed = false;
	bool dragSelect = false;

private:
	Mouse();
	~Mouse() = default;

	void CalculateGameCoordinates();

	sf::RectangleShape selectionBox;
	Unique<Orders> orders;

	bool selectGameItems = false;
	bool suppressed = false;

	float dragX = 0;
	float dragY = 0;

	float gameX = 0;
	float gameY = 0;

	float selectionBoxX = 0;
	float selectionBoxY = 0;
	float selectionBoxWidth = 0;
	float selectionBoxHeight = 0;
};

} // namespace TGX
