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
    EXPECT_EQ(State::Direction(1, 1), state.motionH.d);
    EXPECT_EQ(State::Position(PInitialPosition::X, PInitialPosition::Y), state.motionP.s);
    EXPECT_EQ(State::Direction(0, 0), state.motionP.d);
    EXPECT_EQ(Vector2<int>(0, 0), state.board.mins);
    EXPECT_EQ(Vector2<int>(BoardSizeX, BoardSizeY), state.board.maxs);
    EXPECT_EQ(0, state.simTime);
    EXPECT_TRUE(state.walls.empty());
  }
}

TEST(evasion_core, PlyH)
{
  // Test simple motion without walls.
  {
    SCOPED_TRACE("H - Simple motion");
    State state;
    Initialize(3, 3, &state);
    const State stateBeforePly = state;
    StepH emptyStep;
    PlyError err = DoPly(emptyStep, &state);
    EXPECT_EQ(PlyError::Success, err);
    EXPECT_EQ(stateBeforePly.motionH.s + stateBeforePly.motionH.d, state.motionH.s);
    EXPECT_EQ(stateBeforePly.motionH.d, state.motionH.d);
    EXPECT_EQ(stateBeforePly.simTime + 1, state.simTime);
  }
  // Test collision with vertical wall.
  {
    SCOPED_TRACE("H - Collision vertical wall");
    State state;
    Initialize(3, 3, &state);
    StepH emptyStep;
    // Move once to get space from left side.
    DoPly(emptyStep, &state);
    // Place vertical wall.
    state.walls.push_back(State::Wall());
    {
      const int wallX = state.motionH.s.x + 1;
      typedef State::Wall::Coordinates Coords;
      typedef State::Position Pos;
      State::Wall& wall = state.walls.front();
      // reissb -- 20111030 -- This is just a test scenario, so tag the wall
      //   with an impossible creation time.
      wall.simTimeCreate = -1;
      wall.type = State::Wall::Type_Vertical;
      wall.coords = Coords(Pos(wallX, 0), Pos(wallX, state.board.maxs.y));
    }
    const State stateBeforePly = state;
    PlyError err = DoPly(emptyStep, &state);
    const State::Direction reflectVDir(-stateBeforePly.motionH.d.x,
                                        stateBeforePly.motionH.d.y);
    EXPECT_EQ(PlyError::Success, err);
    EXPECT_EQ(stateBeforePly.motionH.s + reflectVDir, state.motionH.s);
    EXPECT_EQ(reflectVDir, state.motionH.d);
    EXPECT_EQ(stateBeforePly.simTime + 1, state.simTime);
  }
  // Test a collision with 2 board walls.
  {
    SCOPED_TRACE("H - Collision with board");
    State state;
    Initialize(3, 3, &state);
    state.motionH.d = State::Direction(1, 1);
    state.motionH.s = state.board.maxs;
    const State stateBeforePly = state;
    StepH emptyStep;
    PlyError err = DoPly(emptyStep, &state);
    EXPECT_EQ(PlyError::Success, err);
    EXPECT_EQ(stateBeforePly.motionH.s - stateBeforePly.motionH.d, state.motionH.s);
    EXPECT_EQ(-stateBeforePly.motionH.d, state.motionH.d);
    EXPECT_EQ(stateBeforePly.simTime + 1, state.simTime);
  }
}

}

#endif //_HPS_EVASION_EVASION_CORE_GTEST_H_
