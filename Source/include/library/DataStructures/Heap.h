#pragma once

#include <cstddef>
#include <sstream>
#include <type_traits>
#include "Logs.h"
#include "Node/Node.h"

namespace TGX
{
template <typename T>
class Heap
{
private:
	T *items;
	int currentItemCount = 0;
	size_t capacity = 0;

public:
	Heap(size_t size) : capacity(size)
	{
		items = new T[size];
	}

	~Heap()
	{
		Log::Print("Delete Heap");
		delete[] items;
	}

	void Add(T item)
	{
		// Refuse rather than run off the end of the array. A search that pushes
		// more entries than it reserved would otherwise corrupt neighbouring
		// memory and scramble the ordering, which surfaces as erratic paths
		// long before it surfaces as a crash.
		if (static_cast<size_t>(currentItemCount) >= capacity)
		{
			Log::Error("Heap is full, item rejected. Increase the reserved size.");
			return;
		}

		items[currentItemCount] = item;
		items[currentItemCount]->heapIndex = currentItemCount;

		SortUp(items[currentItemCount]);
		currentItemCount++;
	}

	T RemoveFirst()
	{
		if (currentItemCount == 0)
		{
			Log::Error("Attempted to remove from an empty heap!");
			return nullptr;
		}

		T firstItem = items[0];
		currentItemCount--;

		if (currentItemCount > 0)
		{
			items[0] = items[currentItemCount];
			items[0]->heapIndex = 0;
			SortDown(items[0]);
		}

		return firstItem;
	}

	void UpdateItem(T item)
	{
		SortUp(item);
	}

	void SortDown(T &item)
	{
		int markedIndex = item->heapIndex;

		while (true)
		{
			int childIndexLeft = (markedIndex * 2) + 1;
			int childIndexRight = (markedIndex * 2) + 2;
			int swapIndex = 0;

			if (childIndexLeft < currentItemCount)
			{
				swapIndex = childIndexLeft;

				if (childIndexRight < currentItemCount)
				{
					if ((items[childIndexLeft]->CompareTo(items[childIndexRight]) > 0))
					{
						swapIndex = childIndexRight;
					}
				}

				if (items[markedIndex]->CompareTo(items[swapIndex]) > 0)
				{
					SwapDown(markedIndex, swapIndex);
					markedIndex = swapIndex;
				}
				else
				{
					return;
				}
			}
			else
			{
				return;
			}
		}
	}

	void SortUp(T &item)
	{
		int markedIndex = item->heapIndex;
		int parentIndex = ((item->heapIndex - 1) / 2);

		while (true)
		{
			T &parentItem = items[parentIndex];

			if (items[markedIndex]->CompareTo(parentItem) < 0)
			{
				SwapUp(markedIndex, parentIndex);
				markedIndex = parentIndex;
			}
			else
			{
				break;
			}

			parentIndex = ((markedIndex - 1) / 2);
		}
	}

	void SwapUp(int itemA, int itemB)
	{
		float tempF = items[itemA]->f;
		float tempH = items[itemA]->h;
		float tempG = items[itemA]->g;
		int tempX = items[itemA]->x;
		int tempY = items[itemA]->y;
		Node *tempP = items[itemA]->parent;

		items[itemA]->f = items[itemB]->f;
		items[itemA]->h = items[itemB]->h;
		items[itemA]->g = items[itemB]->g;
		items[itemA]->x = items[itemB]->x;
		items[itemA]->y = items[itemB]->y;
		items[itemA]->parent = items[itemB]->parent;

		items[itemB]->f = tempF;
		items[itemB]->h = tempH;
		items[itemB]->g = tempG;
		items[itemB]->x = tempX;
		items[itemB]->y = tempY;
		items[itemB]->parent = tempP;
	}

	void SwapDown(int itemA, int itemB)
	{
		float tempF = items[itemA]->f;
		float tempH = items[itemA]->h;
		float tempG = items[itemA]->g;
		int tempX = items[itemA]->x;
		int tempY = items[itemA]->y;
		Node *tempP = items[itemA]->parent;

		items[itemA]->f = items[itemB]->f;
		items[itemA]->h = items[itemB]->h;
		items[itemA]->g = items[itemB]->g;
		items[itemA]->x = items[itemB]->x;
		items[itemA]->y = items[itemB]->y;
		items[itemA]->parent = items[itemB]->parent;

		items[itemB]->f = tempF;
		items[itemB]->h = tempH;
		items[itemB]->g = tempG;
		items[itemB]->x = tempX;
		items[itemB]->y = tempY;
		items[itemB]->parent = tempP;
	}

	int Count() { return currentItemCount; }
	bool Empty() { return currentItemCount == 0; }

	void Clear()
	{
		for (size_t i = 0; i < static_cast<size_t>(currentItemCount); i++)
		{
			delete items[i];
		}
		currentItemCount = 0;
	}

	void Display()
	{
		std::stringstream ss;
		for (int i = 0; i < currentItemCount; i++)
		{
			ss << "{" << items[i]->f << ", " << items[i]->heapIndex << "}" << " ";
		}
		Log::Print(ss.str());
	}

	void DisplayAll() { Display(); }

	bool Contains(T key)
	{
		for (int i = currentItemCount - 1; i >= 0; i--)
		{
			if ((key->x + (key->y * 60)) == (items[i]->x + (items[i]->y * 60)))
			{
				return true;
			}
		}
		return false;
	}
};
} // namespace TGX
