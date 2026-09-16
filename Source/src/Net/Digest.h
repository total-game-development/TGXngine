#pragma once

#include <cstdint>
#include <cstring>
#include "Core.h"

namespace TGX::Net
{
// The fold both sides of a match use. The server folds the same way over its own
// state -- see Simulation.h in TGXngineServer -- so a number that differs is a
// world that differs, not an implementation that differs.
class Digest
{
private:
	static constexpr std::uint64_t OFFSET = 0xCBF29CE484222325ULL;
	static constexpr std::uint64_t PRIME = 0x100000001B3ULL;

	std::uint64_t fold = OFFSET;

public:
	void Reset()
	{
		fold = OFFSET;
	}

	void Mix(std::uint64_t value)
	{
		fold ^= value;
		fold *= PRIME;
	}

	// Byte by byte rather than through std::hash, whose result differs between
	// standard libraries. A client built against a different one has to arrive
	// at the same number or every check reads as a desync.
	void MixText(const String &text)
	{
		for (unsigned char letter : text)
		{
			Mix(letter);
		}
	}

	// The raw bits, because two clients in step agree exactly. Rounding here
	// would hide the drift this is here to find.
	void MixFloat(float value)
	{
		std::uint32_t bits = 0;

		std::memcpy(&bits, &value, sizeof(bits));

		Mix(bits);
	}

	std::uint64_t Value() const
	{
		return fold;
	}
};
} // namespace TGX::Net
