#ifndef _HPS_EVASION_EVASION_CORE_GTEST_H_
#define _HPS_EVASION_EVASION_CORE_GTEST_H_
#include "evasion_core.h"
#include "gtest/gtest.h"

namespace _hps_evasion_evasion_core_gtest_h_
{
using namespace hps;

TEST(evasion_core, CoreObject)
{
  {
    SCOPED_TRACE("State");
    State state;
    EXPECT_EQ(State::Position(HInitialPosition::X, HInitialPosition::Y), state.motionH.s);
    EXPECT_EQ(State::Direction(0, 0), state.motionH.d);
    EXPECT_EQ(State::Position(PInitialPosition::X, PInitialPosition::Y), state.motionP.s);
    EXPECT_EQ(State::Direction(0, 0), state.motionP.d);
    EXPECT_EQ(0, state.simTime);
    EXPECT_TRUE(state.walls.empty());
    EXPECT_TRUE(state.walls.empty());
  }
}

}

#endif //_HPS_EVASION_EVASION_CORE_GTEST_H_
