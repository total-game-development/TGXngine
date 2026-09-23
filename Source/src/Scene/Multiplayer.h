#pragma once

#include <SFML/Graphics.hpp>
#include "Scene.h"

namespace TGX
{
// The lobby, in two views. The list picks a room; the room picks a seat, a side
// and a map, and says when everybody is ready. Which one is drawn follows the
// session's phase rather than a flag kept here, so a match that starts, ends or
// is refused moves the scene with it.
//
// The arena is the same list with nobody to seat: a room picked there is
// watched while the host's AI plays every side of it.
class Multiplayer : public Scene
{
private:
	// What a click landed on. The scene draws its own controls, so the hit
	// regions are collected as it draws and read back when the click arrives.
	enum class Target : std::uint8_t
	{
		None,
		Room,
		Observe,
		Arena,
		Seat,
		Team,
		Level,
		Ready,
		Back
	};

	struct Hit
	{
		sf::FloatRect bounds;
		Target target = Target::None;
		int value = 0;
	};

	sf::Font font;

	Vector<Hit> hits;

	int hovered = -1;
	int scroll = 0;

	bool loaded = false;
	bool arena = false;

	void Add(const sf::FloatRect &bounds, Target target, int value);

	void DrawHeading();
	void DrawRooms();
	void DrawRoom();
	float DrawLevels(float y);
	void DrawNotice();

	// One labelled box, returned so the caller can keep laying out beneath it.
	void DrawButton(const sf::FloatRect &bounds, const String &label, bool active, bool enabled, int index);

	bool InRoom() const;

public:
	explicit Multiplayer(bool inArena);
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
