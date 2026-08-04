#pragma once

#include <gtest/gtest.h>
#include "DataStructures/Tree.h"

#define Expect_TreeInsertContains(value) \
	tree.Insert(value);                  \
	EXPECT_TRUE(tree.Contains(value));

#define Expect_TreeRemoveContains(value) \
	tree.Remove(value);                  \
	EXPECT_FALSE(tree.Contains(value));

TEST(Tree, InsertRemoveContainsSize)
{
	TGX::Tree<int> tree;

	// remove root
	tree.Insert(1);
	tree.Remove(1);

	EXPECT_EQ(tree.Size(), 0);
	EXPECT_FALSE(tree.Contains(1));

	// lots of removing
	for (int i = 0; i < 100; i++)
	{
		tree.Insert(i);
	}
	for (int i = 0; i < 100; i++)
	{
		tree.Remove(i);
	}
	EXPECT_EQ(tree.Size(), 0);

	// simple inserting
	Expect_TreeInsertContains(1);
	Expect_TreeInsertContains(3);
	Expect_TreeInsertContains(2);
	Expect_TreeInsertContains(-1);
	Expect_TreeInsertContains(0);
	Expect_TreeInsertContains(4);
	Expect_TreeInsertContains(-4);

	// simple size
	EXPECT_EQ(tree.Size(), 7);

	// inserting existing value
	tree.Insert(0);
	EXPECT_EQ(tree.Size(), 7);

	// contains non existing
	EXPECT_FALSE(tree.Contains(-2));
	EXPECT_FALSE(tree.Contains(5));

	// simple removing
	Expect_TreeRemoveContains(3);
	Expect_TreeRemoveContains(0);
	Expect_TreeRemoveContains(-1);

	// size after removing
	EXPECT_EQ(tree.Size(), 4);
}

#undef Expect_TreeInsertContains
#undef Expect_TreeRemoveContains

#define EXPECT_TreeShape(func, values, expected) \
	for (const auto &value : (values))           \
	{                                            \
		tree.Insert(value);                      \
	}                                            \
	EXPECT_EQ(tree.func(), expected);            \
	for (const auto &value : (values))           \
	{                                            \
		tree.Remove(value);                      \
	}

#define list std::initializer_list<int>

TEST(Tree, Shape)
{
	TGX::Tree<int> tree;

	EXPECT_TreeShape(
		ToString,
		(list{1, 3, 2, -1, 0, 4, -4}),
		"R----2(Black)\n"
		"     L----0(Red)\n"
		"     |    L-----1(Black)\n"
		"     |    |    L-----4(Red)\n"
		"     |    R----1(Black)\n"
		"     R----3(Black)\n"
		"          R----4(Red)\n");

	EXPECT_TreeShape(
		ToString,
		(list{1, 2, 3, 4, 5, 6}),
		"R----2(Black)\n"
		"     L----1(Black)\n"
		"     R----4(Red)\n"
		"          L----3(Black)\n"
		"          R----5(Black)\n"
		"               R----6(Red)\n");

	EXPECT_TreeShape(
		ToString,
		(list{6, 5, 4, 3, 2, 1}),
		"R----5(Black)\n"
		"     L----3(Red)\n"
		"     |    L----2(Black)\n"
		"     |    |    L----1(Red)\n"
		"     |    R----4(Black)\n"
		"     R----6(Black)\n");
}

TEST(Tree, Inorder)
{
	TGX::Tree<int> tree;

	EXPECT_TreeShape(
		ToStringInorder,
		(list{1, 3, 2, -1, 0, 4, -4}),
		"{-4} {-1} {0} {1} {2} {3} {4} ");

	EXPECT_TreeShape(
		ToStringInorder,
		(list{1, 2, 3, 4, 5, 6}),
		"{1} {2} {3} {4} {5} {6} ");

	EXPECT_TreeShape(
		ToStringInorder,
		(list{6, 5, 4, 3, 2, 1}),
		"{1} {2} {3} {4} {5} {6} ");
}

TEST(Tree, Preorder)
{
	TGX::Tree<int> tree;

	EXPECT_TreeShape(
		ToStringPreorder,
		(list{1, 3, 2, -1, 0, 4, -4}),
		"{2} {0} {-1} {-4} {1} {3} {4} ");

	EXPECT_TreeShape(
		ToStringPreorder,
		(list{1, 2, 3, 4, 5, 6}),
		"{2} {1} {4} {3} {5} {6} ");

	EXPECT_TreeShape(
		ToStringPreorder,
		(list{6, 5, 4, 3, 2, 1}),
		"{5} {3} {2} {1} {4} {6} ");
}

TEST(Tree, Postorder)
{
	TGX::Tree<int> tree;

	EXPECT_TreeShape(
		ToStringPostorder,
		(list{1, 3, 2, -1, 0, 4, -4}),
		"{-4} {-1} {1} {0} {4} {3} {2} ");

	EXPECT_TreeShape(
		ToStringPostorder,
		(list{1, 2, 3, 4, 5, 6}),
		"{1} {3} {6} {5} {4} {2} ");

	EXPECT_TreeShape(
		ToStringPostorder,
		(list{6, 5, 4, 3, 2, 1}),
		"{1} {2} {4} {3} {6} {5} ");
}

#undef list
#undef EXPECT_TreeShape
