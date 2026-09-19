#pragma once

#include "Core.h"

namespace TGX
{
// Runs a recorded match with no window, the way a networked client runs one,
// and holds its world against every checksum the clients reported. Returns 0
// when every one of them agrees, 1 when any does not, 2 when the recording
// cannot be read.
int RunReplay(const String &path);
} // namespace TGX
