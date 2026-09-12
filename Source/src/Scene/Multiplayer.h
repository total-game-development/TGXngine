#pragma once

#include <SFML/Graphics.hpp>
#include "Scene.h"

namespace TGX
{
class Multiplayer : public Scene
{
private:
	sf::Font font;

	Vector<sf::FloatRect> roomRows;
	sf::FloatRect backButton;

	int hovered = -1;
	int scroll = 0;

	bool loaded = false;

	void DrawHeading();
	void DrawRooms();
	void DrawNotice();

public:
	Multiplayer();
	~Multiplayer() override;

	void Init() override;
	void Update() override;
	void Draw() override;
	void Click() override;
	void RightClick() override;
	void Release() override;
	void Close() override;
	void Free() override;

	bool Key(int code);
};
} // namespace TGX
