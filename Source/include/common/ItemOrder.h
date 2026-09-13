#pragma once

#include "ItemInstance.h"

namespace TGX
{
struct ItemOrder
{
	static bool Before(const ItemInstance &a, const ItemInstance &b)
	{
		if (a.GetPriority() != b.GetPriority())
		{
			return a.GetPriority() < b.GetPriority();
		}

		return a.GetUid() < b.GetUid();
	}

	static bool Before(const ItemInstance *a, const ItemInstance *b)
	{
		if (!a || !b)
		{
			return b != nullptr;
		}

		return Before(*a, *b);
	}
};
} // namespace TGX
