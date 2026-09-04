#pragma once

#include "Core.h"

namespace TGX
{
enum class Alignment : std::uint8_t
{
	None,
	Horizontal,
	Vertical,
	Both
};

inline Alignment AlignmentFromString(const String &str)
{
	if (str == "horizontal") { return Alignment::Horizontal; }
	if (str == "vertical") { return Alignment::Vertical; }
	if (str == "both") { return Alignment::Both; }
	return Alignment::None;
}

enum class SceneType : std::uint8_t
{
	Unknown,
	Intro,
	Skirmish,
	Game
};

inline SceneType SceneTypeFromString(const String &str)
{
	if (str == "intro") { return SceneType::Intro; }
	if (str == "skirmish") { return SceneType::Skirmish; }
	if (str == "game") { return SceneType::Game; }
	return SceneType::Unknown;
}

enum class ResourceType : std::uint8_t
{
	Unknown,
	Obstruction
};

inline ResourceType ResourceTypeFromString(const String &str)
{
	if (str == "obstruction") { return ResourceType::Obstruction; }
	return ResourceType::Unknown;
}

enum class UIAction : std::uint8_t
{
	Unknown,
	None,
	SetSelectedGame,
	Build,
	LoadScene,
	Cancel,
	JoinGame,
	Log,
	Print,
	Send,
	AddGameItem,
	RemoveGameItem,
	GameOver
};

inline UIAction UIActionFromString(const String &str)
{
	if (str == "" || str == "none") { return UIAction::None; }
	if (str == "SetSelectedGame") { return UIAction::SetSelectedGame; }
	if (str == "build") { return UIAction::Build; }
	if (str == "LoadScene") { return UIAction::LoadScene; }
	if (str == "Cancel") { return UIAction::Cancel; }
	if (str == "JoinGame") { return UIAction::JoinGame; }
	if (str == "Log") { return UIAction::Log; }
	if (str == "Print") { return UIAction::Print; }
	if (str == "Send") { return UIAction::Send; }
	if (str == "AddGameItem") { return UIAction::AddGameItem; }
	if (str == "RemoveGameItem") { return UIAction::RemoveGameItem; }
	if (str == "GameOver") { return UIAction::GameOver; }
	return UIAction::Unknown;
}

inline String UIActionToString(UIAction action)
{
	switch (action)
	{
		case UIAction::SetSelectedGame:
			return "SetSelectedGame";
		case UIAction::Build:
			return "build";
		case UIAction::LoadScene:
			return "LoadScene";
		case UIAction::Cancel:
			return "Cancel";
		case UIAction::JoinGame:
			return "JoinGame";
		case UIAction::Log:
			return "Log";
		case UIAction::Print:
			return "Print";
		case UIAction::Send:
			return "Send";
		case UIAction::AddGameItem:
			return "AddGameItem";
		case UIAction::RemoveGameItem:
			return "RemoveGameItem";
		case UIAction::GameOver:
			return "GameOver";
		default:
			return "none";
	}
}
} // namespace TGX
