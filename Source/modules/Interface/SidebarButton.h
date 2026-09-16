#pragma once

#include <SFML/Graphics.hpp>
#include <nlohmann/json.hpp>
#include <Common.hpp>
#include <cstdint>
#include "Core.h"

using namespace nlohmann;

namespace TGX
{
struct SidebarButtonData
{
	String image;
	int frame = 0;
	int frames = 0;
	String extension = ".png";
};

class SidebarButton
{
public:
	enum class States : uint8_t
	{
		Off = 0,
		On = 1,
		Progress = 2,
		Placement = 3,
		Wait = 4,
		Ready = 5,
		Pending = 6
	};
	States buttonState = States::Off;
	States drawState = States::Off;

private:
	UIAction action = UIAction::None;
	String value;
	String type;
	String attached;
	float x;
	float y;
	int width;
	int height;
	Vector<sf::Sprite> sprites;
	Vector<Unique<sf::Texture>> spriteTextures;
	sf::RectangleShape buildableCells;
	int frame;
	int frames;
	float durationCounter;
	float duration;
	int cost;
	int powerUsage;
	bool waitForClick;

	// A button with a buildable grid is one the player puts down by clicking a
	// spot. Kept separately from waitForClick so the two can be held against
	// each other: a placeable button that skips placement has nowhere to put
	// what it built.
	bool placeable = false;

public:
	SidebarButton(SidebarButton &&) = default;
	SidebarButton &operator=(SidebarButton &&) = default;

	SidebarButton(const SidebarButton &) = delete;
	SidebarButton &operator=(const SidebarButton &) = delete;

	SidebarButton();
	~SidebarButton();
	bool Click();
	void Draw();
	void DrawPlacement();
	void Update();
	void SetButton(const String &text, UIAction action, String value, String type, String attached, bool wait, const String &alignment, float x, float y, int width, int height, int xOffset, int yOffset);
	void Clear();
	void SetFrame(int frame);
	void SetFrames(int frames);
	void SetDuration(float duration);
	void SetCost(int cost);
	void SetPowerUsage(int powerUsage);
	void AddBuildableGrid(json buildableGrid);
	void AssignAdditionalButton(const String &image_filename, const String &extension);
	UIAction GetAction();
	String GetValue();
	String GetType();
	int GetCost();
	int GetPowerUsage();
	void ResetButtonState();
	void ResetDrawState();
	void BeginProgress();
	void CancelPending();

	// Whether what is being placed would fit where the cursor is now. Asked
	// again on the click rather than read back from what the last frame drew:
	// the pointer moves between the two, and the answer the frame arrived at
	// was for where it used to be.
	bool PlacementFits() const;

	bool IsPlacing() const
	{
		return buttonState == States::Placement;
	}
	bool HasFreeDeployBerth() const;

private:
	void BuildImmediately();
};
} // namespace TGX
