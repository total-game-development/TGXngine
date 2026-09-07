#pragma once

#include <cstdint>
#include "Core.h"

namespace TGX::Shell
{
enum class NodeKind : std::uint8_t
{
	Program,
	VariableDeclaration,
	FunctionDeclaration,
	AssignmentExpression,
	MemberExpression,
	CallExpression,
	ObjectLiteral,
	ArrayLiteral,
	NumericLiteral,
	StringLiteral,
	Identifier,
	BinaryExpression,
	LogicalExpression,
	UnaryExpression,
	PrintStatement,
	ToggleStatement,
	SplitStatement,
	LengthStatement,
	ReadStatement,
	WriteStatement,
	ExecStatement,
	SpawnStatement,
	ReturnStatement,
	IfStatement,
	WhileStatement,
	ForStatement,
	BreakStatement,
	ContinueStatement,
	Empty
};

struct Node;
using NodeRef = Ref<Node>;

struct Property
{
	String key;
	NodeRef value;
};

struct Node
{
	NodeKind kind = NodeKind::Empty;

	Vector<NodeRef> body;

	String symbol;
	String identifier;
	String op;
	double number = 0.0;
	bool constant = false;
	bool computed = false;

	NodeRef left;
	NodeRef right;
	NodeRef value;
	NodeRef assigne;
	NodeRef object;
	NodeRef property;
	NodeRef caller;
	NodeRef file;
	NodeRef source;
	NodeRef name;
	NodeRef active;
	NodeRef delimiter;
	NodeRef condition;
	NodeRef initializer;
	NodeRef increment;

	Vector<NodeRef> args;
	Vector<NodeRef> elements;
	Vector<NodeRef> thenBranch;
	Vector<NodeRef> elseBranch;
	Vector<Property> properties;
	Vector<String> parameters;
};

inline NodeRef MakeNode(NodeKind kind)
{
	NodeRef node = std::make_shared<Node>();
	node->kind = kind;
	return node;
}
} // namespace TGX::Shell
