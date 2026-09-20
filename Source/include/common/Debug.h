#pragma once

namespace TGX::Debug
{
inline bool showGrid = false;
inline bool showWayPoints = false;
inline bool showEconomy = false;

// Production is what a player sees, so nothing meant for whoever is building
// the game is drawn over it: the toggles still flip, and nothing comes up.
inline bool suppressed = false;

// The frame rate, asked for with --fps. Not an overlay in the sense the others
// are: it is worth having in a production build too, so suppression leaves it
// alone and only the flag decides.
inline bool showFps = false;

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

inline bool Fps(bool debugOnScreen)
{
	return showFps || debugOnScreen;
}
} // namespace TGX::Debug
