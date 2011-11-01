#ifndef _HPS_EVASION_PREY_GTEST_
#define _HPS_EVASION_PREY_GTEST_
#include "prey.h"
#include "evasion_core.h"
#include "evasion_core_gtest.h"
#include "gtest/gtest.h"

namespace prey_gtest
{
  using namespace hps::evasion;
  TEST(RandomPrey, NextStep)
  {
    State state;
    Initialize(3, 3, &state);

    RandomPrey p;
    p.NextStep(state);
  }

  TEST(ScaredPrey, NextStep)
  {
    State state;
    Initialize(3, 3, &state);

    ScaredPrey p;
    StepP step = p.NextStep(state);

    EXPECT_EQ(step.moveDir.x, 1);
    EXPECT_EQ(step.moveDir.y, -1);
  }
}
#endif //_HPS_EVASION_prey_GTEST_
