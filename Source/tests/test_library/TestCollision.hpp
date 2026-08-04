#pragma once

#include <gtest/gtest.h>
#include <cfloat>
#include "Collision/Collision.h"

// for collision tests
// ─ │ ┌ ┐ └ ┘ ├ ┤ ┬ ┴ ┼ ╱ ╲ ╳ ╴ ╵ ╶ ╷

// NOTE: current collision doesnt find the deepest collision, just the first

TEST(Collision, SATCollision)
{
	// simple collision
	// ┌────┐
	// │    │
	// │ ┌──┼─┐
	// │ │  │ │
	// │ │  │ │
	// └─┼──┘ │
	//   │    │
	//   └────┘
	EXPECT_NEAR(TGX::SATCollision({0, 0, 5, 0, 5, 5, 0, 5}, {2, 2, 7, 2, 7, 7, 2, 7}), 3, FLT_EPSILON);
	// backwards collision
	// as above but swapped
	EXPECT_NEAR(TGX::SATCollision({2, 2, 7, 2, 7, 7, 2, 7}, {0, 0, 5, 0, 5, 5, 0, 5}), 3, FLT_EPSILON);
	// complex collision
	//       a  b
	//         ╱  ╲
	// a      ╱     b
	//       b   a ╱
	//         ╲  ╱
	//     a     b
	EXPECT_NEAR(TGX::SATCollision({0, 2, 6, 0, 10, 3, 4, 5}, {9, 0, 13, 2, 10, 5, 6, 3}), 3.2, 0.000001); // FIXME: precision
	// tiny collision
	//      ˇ slightly right
	// ┌────┬────┐
	// │    │    │
	// │    │    │
	// └────┴────┘
	//      ^ slightly up left
	EXPECT_NEAR(TGX::SATCollision({0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f}, {0.999f, 0.001f, 1.001f, 1.0f, 2.0f, 1.0f, 2.0f, 0.0f}), 0.001, FLT_EPSILON);
	// same position
	// ┌─┐
	// └─┘
	EXPECT_NEAR(TGX::SATCollision({0, 0, 1, 0, 1, 1, 0, 1}, {0, 0, 1, 0, 1, 1, 0, 1}), 1.0, FLT_EPSILON);
	// completely inside (technically not colliding)
	// ┌──────┐
	// │  ┌─┐ │
	// │  └─┘ │
	// └──────┘
	EXPECT_NEAR(TGX::SATCollision({0, 0, 7, 0, 7, 3, 0, 3}, {1, 3, 3, 3, 3, 4, 1, 4}), 0.0, FLT_EPSILON);
}
