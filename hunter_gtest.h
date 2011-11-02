#ifndef _HPS_EVASION_HUNTER_GTEST_
#define _HPS_EVASION_HUNTER_GTEST_
#include "hunter.h"
#include "evasion_core.h"
#include "evasion_core_gtest.h"
#include "gtest/gtest.h"

namespace hunter_gtest
{
  using namespace hps::evasion;
  /*TEST(BasicHunter, Play)
  {
    State state;
    Initialize(3, 3, &state);

    BasicHunter h;
    
    // If the hunter is far from the prey, it should do nothing.
    ASSERT_TRUE(state.motionH.pos.x == InitialPosition::Hunter::X);
    ASSERT_TRUE(state.motionH.pos.y == InitialPosition::Hunter::Y);
    ASSERT_TRUE(state.posStackP.back().x == InitialPosition::Prey::X);
    ASSERT_TRUE(state.posStackP.back().y == InitialPosition::Prey::Y);

    ASSERT_TRUE(state.walls.size() == 0);
    ASSERT_TRUE(state.simTime == 0);

    h.Play(state);
    ASSERT_TRUE(state.walls.size() == 0);
    ASSERT_TRUE(state.simTime == 2);


  }*/
}
#endif //_HPS_EVASION_HUNTER_GTEST_
