#pragma once

#include <gtest/gtest.h>
#include <cfloat>
#include "Heuristic/Heuristic.h"

TEST(Heuristic, Manhattan)
{
	// same point
	EXPECT_NEAR(TGX::Manhattan(0, 0, 0, 0), 0, FLT_EPSILON);
	// simple
	EXPECT_NEAR(TGX::Manhattan(0, 0, 1, 1), 2, FLT_EPSILON);
	// negative
	EXPECT_NEAR(TGX::Manhattan(-1, -2, -3, -4), 4, FLT_EPSILON);
	// same but flipped
	EXPECT_NEAR(TGX::Manhattan(-4, -3, -2, -1), 4, FLT_EPSILON);
	// complex
	EXPECT_NEAR(TGX::Manhattan(23567, -33526, 234542, 123758), 368259, FLT_EPSILON);
}

TEST(Heuristic, Euclidean)
{
	// same point
	EXPECT_NEAR(TGX::Euclidean(0, 0, 0, 0), 0, FLT_EPSILON);
	// simple
	EXPECT_NEAR(TGX::Euclidean(0, 0, 1, 1), 1.4142135623731, FLT_EPSILON);
	// negative
	EXPECT_NEAR(TGX::Euclidean(-1, -2, -3, -4), 2.8284271247462, FLT_EPSILON);
	// same but flipped
	EXPECT_NEAR(TGX::Euclidean(-4, -3, -2, -1), 2.8284271247462, FLT_EPSILON);
	// complex
	EXPECT_NEAR(TGX::Euclidean(23567, -33526, 234542, 123758), 263151.5, FLT_EPSILON);
}
