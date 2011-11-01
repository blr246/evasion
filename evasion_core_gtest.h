#ifndef _HPS_EVASION_EVASION_CORE_GTEST_H_
#define _HPS_EVASION_EVASION_CORE_GTEST_H_
#include "evasion_core.h"
#include "gtest/gtest.h"
#include <sstream>

namespace hps
{
namespace evasion
{
/// <summary> An equality operator for State testing. </summary>
inline bool operator==(const State::MotionInfo& lhs,
                       const State::MotionInfo& rhs)
{
  return (lhs.s == rhs.s) && (lhs.d == rhs.d);
}
/// <summary> An equality operator for State testing. </summary>
inline bool operator==(const State& lhs, const State& rhs)
{
  return (lhs.simTime == rhs.simTime) &&
         (lhs.simTimeLastWall == rhs.simTimeLastWall) &&
         (lhs.motionP == rhs.motionP) &&
         (lhs.motionH == rhs.motionH) &&
         (lhs.walls == rhs.walls) &&
         (lhs.board == rhs.board) &&
         (lhs.wallCreatePeriod == rhs.wallCreatePeriod) &&
         (lhs.maxWalls == rhs.maxWalls) &&
         (lhs.preyCaptureInfo == rhs.preyCaptureInfo);
}
}
}

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
    EXPECT_EQ(Vector2<int>(BoardSizeX - 1, BoardSizeY - 1), state.board.maxs);
    EXPECT_EQ(0, state.simTime);
    EXPECT_TRUE(state.walls.empty());
  }
}

/// <summary> Test motion interaction with a single wall. </summary>
void PlyHSingleWallTestAllDir(const State::Wall& hit, const bool isBorder)
{
  assert(WallValid(hit));
  std::stringstream ssTrace;
  ssTrace << "H - Bounce wall " << hit.coords.p0 << " -> "
          << hit.coords.p1 << ", "
          << ((State::Wall::Type_Horizontal == hit.type) ? "Type_Horizontal" :
                                                           "Type_Vertical");
  SCOPED_TRACE(ssTrace.str());
  State state;
  Initialize(3, 3, &state);
  if (!isBorder)
  {
    state.walls.push_back(hit);
  }
  // Place H at center of wall and find in bounds coordinate.
  State::Position& posH = state.motionH.s;
  posH = (hit.coords.p0 + hit.coords.p1) / 2;
  if (State::Wall::Type_Horizontal == hit.type)
  {
    // Move +/- Y-coordinate only.
    if ((posH.y - 1) >= state.board.mins.y)
    {
      --posH.y;
    }
    else
    {
      assert((posH.y + 1) <= state.board.maxs.y);
      ++posH.y;
    }
  }
  else
  {
    assert(State::Wall::Type_Vertical == hit.type);
    // Move +/- X-coordinate only.
    if ((posH.x - 1) >= state.board.mins.x)
    {
      --posH.x;
    }
    else
    {
      assert((posH.x + 1) <= state.board.maxs.x);
      ++posH.x;
    }
  }
  // Setup all possible directions.
  static const State::Direction s_dirs[] = { State::Direction( 1,  1),
                                             State::Direction(-1,  1),
                                             State::Direction( 1, -1),
                                             State::Direction(-1, -1) };
  enum { NumDirs = sizeof(s_dirs) / sizeof(s_dirs[0]), };
  const StepH emptyStep;
  for (int dirIdx = 0; dirIdx < NumDirs; ++dirIdx)
  {
    // Set direction.
    state.motionH.d = s_dirs[dirIdx];
    const State beforePly = state;
    // Find reflection mask.
    const State::Position nextPos = state.motionH.s + state.motionH.d;
    Vector2<int> reflectMask(1, 1);
    if ((State::Wall::Type_Horizontal == hit.type) &&
        (nextPos.y == hit.coords.p0.y) &&
        ((hit.coords.p0.x < nextPos.x) && (nextPos.x < hit.coords.p1.x)))
    {
      reflectMask.y = -1; // Hits a horizontal wall -> vertical bounce
    }
    if ((State::Wall::Type_Vertical == hit.type) &&
        (nextPos.x == hit.coords.p0.x) &&
        ((hit.coords.p0.y < nextPos.y) && (nextPos.y < hit.coords.p1.y)))
    {
      reflectMask.x = -1; // Hits a horizontal wall -> vertical bounce
    }
    const State::Direction reflectDir(reflectMask.x * state.motionH.d.x,
                                      reflectMask.y * state.motionH.d.y);
    const State::Direction offset(reflectMask.x > 0 ? state.motionH.d.x : 0,
                                  reflectMask.y > 0 ? state.motionH.d.y : 0);
    const State::Position expectPos = state.motionH.s + offset;
    PlyError err = DoPly(emptyStep, &state);
    EXPECT_EQ(PlyError::Success, err);
    EXPECT_EQ(expectPos, state.motionH.s);
    EXPECT_EQ(reflectDir, state.motionH.d);
    EXPECT_EQ(beforePly.simTime + 1, state.simTime);
    UndoPly(emptyStep, &state);
    EXPECT_EQ(beforePly, state);
  }
}
TEST(evasion_core, PlyH)
{
  // Test simple motion without walls.
  {
    SCOPED_TRACE("H - Simple motion");
    State state;
    Initialize(3, 3, &state);
    const State beforePly = state;
    StepH emptyStep;
    PlyError err = DoPly(emptyStep, &state);
    EXPECT_EQ(PlyError::Success, err);
    EXPECT_EQ(beforePly.motionH.s + beforePly.motionH.d, state.motionH.s);
    EXPECT_EQ(beforePly.motionH.d, state.motionH.d);
    EXPECT_EQ(beforePly.simTime + 1, state.simTime);
    UndoPly(emptyStep, &state);
    EXPECT_EQ(beforePly, state);
  }
  // Test collision with single walls.
  {
    typedef State::Position Pos;
    typedef State::Wall::Coordinates WallCoords;
    {
      SCOPED_TRACE("H - Collision with single board wall");
      State::Wall wall;
      wall.simTimeCreate = -1;
      {
        wall.type = State::Wall::Type_Horizontal;
        wall.coords = WallCoords(Pos(-1, -1), Pos(BoardSizeX, -1));
        PlyHSingleWallTestAllDir(wall, true);
      }
      {
        wall.type = State::Wall::Type_Vertical;
        wall.coords = WallCoords(Pos(BoardSizeX, -1),
                                 Pos(BoardSizeX, BoardSizeY));
        PlyHSingleWallTestAllDir(wall, true);
      }
      {
        wall.type = State::Wall::Type_Horizontal;
        wall.coords = WallCoords(Pos(-1, BoardSizeY),
                                 Pos(BoardSizeX, BoardSizeY));
        PlyHSingleWallTestAllDir(wall, true);
      }
      {
        wall.type = State::Wall::Type_Vertical;
        wall.coords = WallCoords(Pos(-1, -1),
                                 Pos(-1, BoardSizeY));
        PlyHSingleWallTestAllDir(wall, true);
      }
    }
    {
      SCOPED_TRACE("H - Collision with single wall");
      State::Wall wall;
      wall.simTimeCreate = -1;
      {
        wall.type = State::Wall::Type_Horizontal;
        wall.coords = WallCoords(Pos(0, BoardSizeY / 2),
                                 Pos(BoardSizeX, BoardSizeY / 2));
        PlyHSingleWallTestAllDir(wall, false);
      }
      {
        wall.type = State::Wall::Type_Vertical;
        wall.coords = WallCoords(Pos(BoardSizeX / 2, 0),
                                 Pos(BoardSizeX / 2, BoardSizeY));
        PlyHSingleWallTestAllDir(wall, false);
      }
    }
  }
  // Test a collision with 2 board walls.
  {
    SCOPED_TRACE("H - Collision with board corner");
    State state;
    Initialize(3, 3, &state);
    state.motionH.d = State::Direction(1, 1);
    state.motionH.s = state.board.maxs;
    const State beforePly = state;
    StepH emptyStep;
    PlyError err = DoPly(emptyStep, &state);
    EXPECT_EQ(PlyError::Success, err);
    EXPECT_EQ(beforePly.motionH.s, state.motionH.s);
    EXPECT_EQ(-beforePly.motionH.d, state.motionH.d);
    EXPECT_EQ(beforePly.simTime + 1, state.simTime);
    UndoPly(emptyStep, &state);
    EXPECT_EQ(beforePly, state);
  }
  // Test collision with vertical and horizontal wall.
  {
    SCOPED_TRACE("H - Collision with wall corner");
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
      State::Wall& wall = state.walls.back();
      // reissb -- 20111030 -- This is just a test scenario, so tag the wall
      //   with an impossible creation time.
      wall.simTimeCreate = -1;
      wall.type = State::Wall::Type_Vertical;
      wall.coords = Coords(Pos(wallX, 0), Pos(wallX, state.board.maxs.y));
    }
    // Place horizontal wall.
    state.walls.push_back(State::Wall());
    {
      const int wallY = state.motionH.s.y + 1;
      typedef State::Wall::Coordinates Coords;
      typedef State::Position Pos;
      State::Wall& wall = state.walls.back();
      // reissb -- 20111030 -- This is just a test scenario, so tag the wall
      //   with an impossible creation time.
      wall.simTimeCreate = -1;
      wall.type = State::Wall::Type_Horizontal;
      wall.coords = Coords(Pos(0, wallY), Pos(state.board.maxs.y, wallY));
    }
    const State beforePly = state;
    PlyError err = DoPly(emptyStep, &state);
    EXPECT_EQ(PlyError::Success, err);
    EXPECT_EQ(beforePly.motionH.s, state.motionH.s);
    EXPECT_EQ(-beforePly.motionH.d, state.motionH.d);
    EXPECT_EQ(beforePly.simTime + 1, state.simTime);
    UndoPly(emptyStep, &state);
    EXPECT_EQ(beforePly, state);
  }
  // Test collision with one board edge and one wall.
  {
    SCOPED_TRACE("H - Collision with board edge and wall corner");
    State state;
    Initialize(3, 3, &state);
    StepH emptyStep;
    // Move once to get space from left side.
    DoPly(emptyStep, &state);
    // Move to edge of board horizontally.
    state.motionH.s.x = state.board.maxs.x;
    // Place horizontal wall.
    state.walls.push_back(State::Wall());
    {
      const int wallY = state.motionH.s.y + 1;
      typedef State::Wall::Coordinates Coords;
      typedef State::Position Pos;
      State::Wall& wall = state.walls.back();
      // reissb -- 20111030 -- This is just a test scenario, so tag the wall
      //   with an impossible creation time.
      wall.simTimeCreate = -1;
      wall.type = State::Wall::Type_Horizontal;
      wall.coords = Coords(Pos(0, wallY), Pos(state.board.maxs.y, wallY));
    }
    const State beforePly = state;
    PlyError err = DoPly(emptyStep, &state);
    EXPECT_EQ(PlyError::Success, err);
    EXPECT_EQ(beforePly.motionH.s, state.motionH.s);
    EXPECT_EQ(-beforePly.motionH.d, state.motionH.d);
    EXPECT_EQ(beforePly.simTime + 1, state.simTime);
    UndoPly(emptyStep, &state);
    EXPECT_EQ(beforePly, state);
  }
}

}

#endif //_HPS_EVASION_EVASION_CORE_GTEST_H_
