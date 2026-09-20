#pragma once

namespace TGX::Debug
{
inline bool showGrid = false;
inline bool showWayPoints = false;
inline bool showEconomy = false;

// Production is what a player sees, so nothing meant for whoever is building
// the game is drawn over it: the toggles still flip, and nothing comes up.
inline bool suppressed = false;

inline bool Grid()
{
	return showGrid && !suppressed;
}

inline bool WayPoints()
{
	return showWayPoints && !suppressed;
}

inline bool Economy()
{
	return showEconomy && !suppressed;
}
} // namespace TGX::Debug
