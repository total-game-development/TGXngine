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
		Ready = 5
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
	bool HasFreeDeployBerth() const;

private:
	void BuildImmediately();
};
} // namespace TGX
