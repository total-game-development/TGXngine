#pragma once

#include <gtest/gtest.h>
#include "Common.hpp"

// test class since ItemInstance is abstract
class TestItemInstance : public TGX::ItemInstance
{
public:
	TestItemInstance(float x, float y)
	{
		SetX(x);
		SetY(y);
	}
	void AddToGrid(TGX::Vector<TGX::Vector<int>> & /*grid*/, TGX::GridTracker & /*gridTracker*/) const override {}
	void RemoveFromGrid(TGX::Vector<TGX::Vector<int>> & /*grid*/, TGX::GridTracker & /*gridTracker*/) const override {}
	float GetRadius() const override
	{
		return 0.0f;
	}
	int GetFrames() const override
	{
		return 1;
	}
	int GetCellCollisionMode() const override
	{
		return 0;
	}
};

TEST(Common, GetAngleAsDegrees)
{
	TestItemInstance item{0.0, 0.0};

	// east
	EXPECT_FLOAT_EQ(TGX::GetAngleAsDegrees(item, {1, 0}), 0.0f);
	// north
	EXPECT_FLOAT_EQ(TGX::GetAngleAsDegrees(item, {0, 1}), 90.0f);
	// west
	EXPECT_FLOAT_EQ(TGX::GetAngleAsDegrees(item, {-1, 0}), 180.0f);
	// south
	EXPECT_FLOAT_EQ(TGX::GetAngleAsDegrees(item, {0, -1}), 270.0f);
	// diagonal
	EXPECT_FLOAT_EQ(TGX::GetAngleAsDegrees(item, {1, 1}), 45.0f);
	// diagonal
	EXPECT_FLOAT_EQ(TGX::GetAngleAsDegrees(item, {-1, -1}), 225.0f);
}

TEST(Common, AngleDifference)
{
	EXPECT_FLOAT_EQ(TGX::AngleDifference(10, 20), 10);
	EXPECT_FLOAT_EQ(TGX::AngleDifference(350, 10), 20);
	EXPECT_FLOAT_EQ(TGX::AngleDifference(0, 180), 180);
}

TEST(Common, CoordinateToIndex)
{
	EXPECT_EQ(TGX::CoordinateToIndex(0, 0, 10), 0);
	EXPECT_EQ(TGX::CoordinateToIndex(1, 0, 10), 1);
	EXPECT_EQ(TGX::CoordinateToIndex(0, 1, 10), 10);
	EXPECT_EQ(TGX::CoordinateToIndex(3, 2, 10), 23);
}

TEST(Common, WrapDirection)
{
	EXPECT_FLOAT_EQ(TGX::WrapDirection(-1, 8), 7);
	EXPECT_FLOAT_EQ(TGX::WrapDirection(8, 8), 0);
	EXPECT_FLOAT_EQ(TGX::WrapDirection(3, 8), 3);
}

TEST(Common, CalculateAngle)
{
	EXPECT_FLOAT_EQ(TGX::CalculateAngle(0, 8), 0);
	EXPECT_FLOAT_EQ(TGX::CalculateAngle(2, 8), 90);
	EXPECT_FLOAT_EQ(TGX::CalculateAngle(4, 8), 180);
	EXPECT_FLOAT_EQ(TGX::CalculateAngle(6, 8), 270);
}

TEST(Common, FindAngle)
{
	// above
	EXPECT_FLOAT_EQ(TGX::FindAngle(0, 1, 0, 0, 8), 4);
	// right
	EXPECT_FLOAT_EQ(TGX::FindAngle(1, 0, 0, 0, 8), 2);
	// below
	EXPECT_NEAR(TGX::FindAngle(0, -1, 0, 0, 8), 0, 0.000001); // FIXME: precision
	// left
	EXPECT_FLOAT_EQ(TGX::FindAngle(-1, 0, 0, 0, 8), 6);
	// upper right
	EXPECT_FLOAT_EQ(TGX::FindAngle(1, 1, 0, 0, 8), 3);
	// upper left
	EXPECT_FLOAT_EQ(TGX::FindAngle(-1, 1, 0, 0, 8), 5);
	// lower right
	EXPECT_FLOAT_EQ(TGX::FindAngle(1, -1, 0, 0, 8), 1);
	// lower left
	EXPECT_FLOAT_EQ(TGX::FindAngle(-1, -1, 0, 0, 8), 7);
}

TEST(Common, AngleDiff)
{
	// basic forward / backward
	EXPECT_FLOAT_EQ(TGX::AngleDiff(1, 2, 8), 1);
	EXPECT_FLOAT_EQ(TGX::AngleDiff(7, 1, 8), 2);
	EXPECT_FLOAT_EQ(TGX::AngleDiff(1, 7, 8), -2);
	// no difference
	EXPECT_FLOAT_EQ(TGX::AngleDiff(0, 0, 8), 0);
	EXPECT_FLOAT_EQ(TGX::AngleDiff(4, 4, 8), 0);
	// half turn boundaries
	EXPECT_FLOAT_EQ(TGX::AngleDiff(0, 4, 8), -4);
	EXPECT_FLOAT_EQ(TGX::AngleDiff(4, 0, 8), 4);
	// wrap around symmetry
	EXPECT_FLOAT_EQ(TGX::AngleDiff(6, 0, 8), 2);
	EXPECT_FLOAT_EQ(TGX::AngleDiff(0, 6, 8), -2);
	// large direction count
	EXPECT_FLOAT_EQ(TGX::AngleDiff(15, 1, 16), 2);
	EXPECT_FLOAT_EQ(TGX::AngleDiff(1, 15, 16), -2);
}

TEST(Common, DistanceBetweenTwoPoints)
{
	EXPECT_FLOAT_EQ(TGX::DistanceBetweenTwoPoints(0, 0, 3, 4), 5);
	EXPECT_FLOAT_EQ(TGX::DistanceBetweenTwoPoints(1, 1, 1, 1), 0);
}

TEST(Common, Intersect)
{
	TGX::WorldState &world = TGX::WorldState::GetInstance();
	world.SetGameX(50);
	world.SetGameY(50);

	// inside
	EXPECT_TRUE(TGX::Intersect(50, 50, 0, 0, 100, 100));
	// outside bottom right
	EXPECT_FALSE(TGX::Intersect(101, 101, 0, 0, 100, 100));
	// exactly on left edge
	EXPECT_TRUE(TGX::Intersect(0, 50, 0, 0, 100, 100));
	// exactly on top edge
	EXPECT_TRUE(TGX::Intersect(50, 100, 0, 0, 100, 100));
	// just inside
	EXPECT_TRUE(TGX::Intersect(0.01f, 0.01f, 0, 0, 100, 100));
	// outside left
	EXPECT_FALSE(TGX::Intersect(-0.01f, -0.01f, 0, 0, 100, 100));
	// outside top
	EXPECT_FALSE(TGX::Intersect(50, 100.01f, 0, 0, 100, 100));
	// large bounds
	EXPECT_TRUE(TGX::Intersect(0, 0, -100000, -100000, 100000, 100000));

	world.Clear();
}

#define Expect_HoverFromCorner(x, y, expected) \
	world.SetGameX(x);                         \
	world.SetGameY(y);                         \
	EXPECT_EQ(TGX::HoverFromCorner(&item), expected);

TEST(Common, HoverFromCorner)
{
	TestItemInstance item(2, 2);
	item.SetWidth(50);
	item.SetHeight(50);

	TGX::WorldState &world = TGX::WorldState::GetInstance();

	world.SetGameX(50);
	world.SetGameY(50);

	// background Y offset
	world.SetBackgroundOffsetY(20);
	EXPECT_FALSE(TGX::HoverFromCorner(&item));
	world.SetBackgroundOffsetY(100);
	EXPECT_FALSE(TGX::HoverFromCorner(&item));
	world.SetBackgroundOffsetY(0);

	// inside bounds
	Expect_HoverFromCorner(50, 50, true);
	// too far left
	Expect_HoverFromCorner(10, 50, false);
	// too far above
	Expect_HoverFromCorner(50, 5, false);
	// exactly on left edge
	Expect_HoverFromCorner(item.GetX() * 20, 50, true);
	// far outside bottom right
	Expect_HoverFromCorner(500, 500, false);

	world.Clear();
}

#undef Expect_HoverFromCorner

#define Expect_HoverFromCenter(x, y, radius, expected) \
	world.SetGameX((x) * 20.0f);                       \
	world.SetGameY((y) * 20.0f);                       \
	EXPECT_EQ(TGX::HoverFromCenter(&item, radius), expected);

TEST(Common, HoverFromCenter)
{
	TestItemInstance item(30, 30);
	item.SetWidth(40);
	item.SetHeight(40);

	TGX::WorldState &world = TGX::WorldState::GetInstance();

	// inside
	Expect_HoverFromCenter(30, 30, 10, true);

	// too far left
	Expect_HoverFromCenter(-100, 0, 10, false);
	// too far right
	Expect_HoverFromCenter(100, 0, 10, false);
	// too far above
	Expect_HoverFromCenter(0, 100, 10, false);
	// too far below
	Expect_HoverFromCenter(-100, 0, 10, false);

	// larger radius
	Expect_HoverFromCenter(0, 0, 1000, true);

	// zero radius inside
	Expect_HoverFromCenter(30, 30, 0, true);
	// zero radius outside
	Expect_HoverFromCenter(0, 0, 0, false);

	world.Clear();
}

#undef Expect_HoverFromCenter

TEST(Common, Move)
{
	TestItemInstance item(0, 0);
	item.SetDirection(0);

	TGX::Move(64, 8, &item);

	EXPECT_FLOAT_EQ(item.GetX(), 0);
	EXPECT_FLOAT_EQ(item.GetY(), -1);
}

TEST(Common, FindFurthestItem)
{
	TestItemInstance a(0, 0);
	TestItemInstance b(0, 0);

	EXPECT_FALSE(TGX::FindFurthestItem(&a, &b, {1, 0}, {0, 1}));
	EXPECT_FALSE(TGX::FindFurthestItem(&a, &b, {0, 1}, {0, 1}));
	EXPECT_FALSE(TGX::FindFurthestItem(&a, &b, {1, 0}, {1, 0}));
	EXPECT_FALSE(TGX::FindFurthestItem(&a, &b, {0, 1}, {1, 0}));

	// east vs north
	EXPECT_FALSE(TGX::FindFurthestItem(&a, &b, {1, 0}, {0, 1}));
	// northeast vs north
	EXPECT_FALSE(TGX::FindFurthestItem(&a, &b, {1, 1}, {0, 1}));
	// west vs northwest
	EXPECT_FALSE(TGX::FindFurthestItem(&a, &b, {-1, 0}, {-2, 1}));
	// south vs southeast
	EXPECT_FALSE(TGX::FindFurthestItem(&a, &b, {0, -1}, {1, -2}));
}

TEST(Common, FindLongestPath)
{
	TGX::Vector<std::tuple<int, int>> pathA{{0, 0}, {1, 0}};
	TGX::Vector<std::tuple<int, int>> pathB{{0, 0}};

	EXPECT_EQ(TGX::FindLongestPath(1, 2, pathA, pathB, 0, 0, 0, 0, 2, 0, 1, 0), 2);
}
