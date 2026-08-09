#include <gtest/gtest.h>
#include "test_common/TestCommon.hpp"
#include "test_common/TestHeuristic.hpp"
#include "test_common/TestStringUtils.hpp"
#include "test_common/TestTree.hpp"
#include "test_common/TestUtils.hpp"
#include "test_library/TestCollision.hpp"
#include "test_library/TestNavalAStar.hpp"

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
