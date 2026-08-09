#pragma once

#include <cstdint>
#include <functional>

namespace TGX
{
struct Node
{
	enum class NodeType : std::uint8_t
	{
		DefaultConstruct,
		ImportXY,
		ImportXYGHF,
		ImportXYGHFParent,
		CopyConstruct
	};

	int x, y = 0;
	float g, h, f = 0.0;
	Node *parent;
	int heapIndex = 0;
	NodeType type;

	int dx = 0;
	int dy = 0;

	static inline int default_construct_counter;
	static inline int import_x_y_construct_counter;
	static inline int import_x_y_g_h_f_construct_counter;
	static inline int import_x_y_g_h_f_parent_construct_counter;
	static inline int copy_construct_counter;
	static inline int default_destruct_counter;
	static inline int import_x_y_destruct_counter;
	static inline int import_x_y_g_h_f_destruct_counter;
	static inline int import_x_y_g_h_f_parent_destruct_counter;
	static inline int copy_destruct_counter;

	Node() : x(0), y(0), g(0.0), h(0.0), f(0.0), parent(nullptr)
	{
		type = NodeType::DefaultConstruct;
		default_construct_counter++;
	}

	Node(int _x, int _y) : x(_x), y(_y), g(0.0), h(0.0), f(0.0), parent(nullptr)
	{
		type = NodeType::ImportXY;
		import_x_y_construct_counter++;
	}

	Node(int _x, int _y, float _g, float _h, float _f) : x(_x), y(_y), g(_g), h(_h), f(_f), parent(nullptr)
	{
		type = NodeType::ImportXYGHF;
		import_x_y_g_h_f_construct_counter++;
	}

	Node(int _x, int _y, float _g, float _h, float _f, Node *_parent) : x(_x), y(_y), g(_g), h(_h), f(_f), parent(_parent)
	{
		type = NodeType::ImportXYGHFParent;
		import_x_y_g_h_f_parent_construct_counter++;
	}

	Node(const Node &node) : x(node.x), y(node.y), g(node.g), h(node.h), f(node.f), parent(node.parent)
	{
		type = NodeType::CopyConstruct;
		copy_construct_counter++;
	}

	~Node()
	{
		switch (type)
		{
			case NodeType::DefaultConstruct:
				default_destruct_counter++;
				break;
			case NodeType::ImportXY:
				import_x_y_destruct_counter++;
				break;
			case NodeType::ImportXYGHF:
				import_x_y_g_h_f_destruct_counter++;
				break;
			case NodeType::ImportXYGHFParent:
				import_x_y_g_h_f_parent_destruct_counter++;
				break;
			case NodeType::CopyConstruct:
				copy_destruct_counter++;
				break;
		}
	}

	int CompareTo(const Node &other) const
	{
		if (f < other.f)
		{
			return -1;
		}
		if (f > other.f)
		{
			return 1;
		}
		if (h < other.h)
		{
			return -1;
		}
		if (h > other.h)
		{
			return 1;
		}
		return 0;
	}

	int CompareTo(const Node *other) const
	{
		return CompareTo(*other);
	}

	bool operator==(const Node &other) const
	{
		return x == other.x && y == other.y;
	}
};
} // namespace TGX

namespace std
{
template <>
struct hash<TGX::Node>
{
	size_t operator()(const TGX::Node &node) const
	{
		return hash<int>()(node.x) ^ (hash<int>()(node.y) << 1);
	}
};
} // namespace std
