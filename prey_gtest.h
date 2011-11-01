#ifndef _HPS_EVASION_PREY_GTEST_
#define _HPS_EVASION_PREY_GTEST_
#include "prey.h"
#include "evasion_core.h"
#include "evasion_core_gtest.h"
#include "gtest/gtest.h"

namespace prey_gtest
{
  using namespace hps::evasion;
  TEST(RandomPrey, Play)
  {
    State state;
    Initialize(3, 3, &state);

    RandomPrey h;
    
  }
}
#endif //_HPS_EVASION_prey_GTEST_
