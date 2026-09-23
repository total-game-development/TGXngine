#include <gtest/gtest.h>
#include "test_common/TestCommandStrings.hpp"
#include "test_common/TestCommon.hpp"
#include "test_common/TestHeuristic.hpp"
#include "test_common/TestMinimap.hpp"
#include "test_common/TestRandom.hpp"
#include "test_common/TestStringUtils.hpp"
#include "test_common/TestTreasury.hpp"
#include "test_common/TestTree.hpp"
#include "test_common/TestUtils.hpp"
#include "test_library/TestCollision.hpp"
#include "test_library/TestNavalAStar.hpp"
#include "test_net/TestDigest.hpp"
#include "test_net/TestLockstep.hpp"
#include "test_rules/TestRules.hpp"
#include "test_rules/TestSettle.hpp"
#include "test_shell/TestFileSystem.hpp"
#include "test_shell/TestHighlight.hpp"
#include "test_shell/TestInterpreter.hpp"
#include "test_shell/TestPersistence.hpp"
#include "test_shell/TestProcesses.hpp"
#include "test_shell/TestRadar.hpp"
#include "test_shell/TestRemote.hpp"

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
