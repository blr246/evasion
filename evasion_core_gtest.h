#ifndef _HPS_EVASION_EVASION_CORE_GTEST_H_
#define _HPS_EVASION_EVASION_CORE_GTEST_H_
#include "evasion_core.h"
#include "wall_clip_util.h"
#include "gtest/gtest.h"
#include <sstream>

namespace hps
{
namespace evasion
{
/// <summary> An equality operator for State testing. </summary>
inline bool operator==(const State::HunterMotionInfo& lhs,
                       const State::HunterMotionInfo& rhs)
{
  return (lhs.pos == rhs.pos) && (lhs.dir == rhs.dir);
}
/// <summary> An equality operator for State testing. </summary>
inline bool operator==(const State& lhs, const State& rhs)
{
  return (lhs.simTime == rhs.simTime) &&
         (lhs.simTimeLastWall == rhs.simTimeLastWall) &&
         (lhs.posStackP == rhs.posStackP) &&
         (lhs.motionH == rhs.motionH) &&
         (lhs.walls == rhs.walls) &&
         (lhs.board == rhs.board) &&
         (lhs.wallCreatePeriod == rhs.wallCreatePeriod) &&
         (lhs.maxWalls == rhs.maxWalls) &&
         (lhs.preyState == rhs.preyState);
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
    EXPECT_EQ(State::Position(InitialPosition::Hunter::X,
                              InitialPosition::Hunter::Y), state.motionH.pos);
    EXPECT_EQ(State::Direction(1, 1), state.motionH.dir);
    EXPECT_EQ(State::HunterMotionInfo::Flag_NotStuck, state.motionH.simTimeStuck);
    EXPECT_EQ(State::Position(InitialPosition::Prey::X,
                              InitialPosition::Prey::Y), state.posStackP.back());
    EXPECT_EQ(Vector2<int>(0, 0), state.board.mins);
    EXPECT_EQ(Vector2<int>(BoardSizeX - 1, BoardSizeY - 1), state.board.maxs);
    EXPECT_EQ(0, state.simTime);
    EXPECT_TRUE(state.walls.empty());
    EXPECT_EQ(State::PreyState_Evading, state.preyState);
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
  State::Position& posH = state.motionH.pos;
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
    state.motionH.dir = s_dirs[dirIdx];
    const State beforePly = state;
    // Find reflection mask.
    const State::Position nextPos = state.motionH.pos + state.motionH.dir;
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
    const State::Direction reflectDir(reflectMask.x * state.motionH.dir.x,
                                      reflectMask.y * state.motionH.dir.y);
    const State::Direction offset(reflectMask.x > 0 ? state.motionH.dir.x : 0,
                                  reflectMask.y > 0 ? state.motionH.dir.y : 0);
    const State::Position expectPos = state.motionH.pos + offset;
    PlyError err = DoPly(emptyStep, &state);
    EXPECT_EQ(PlyError::Success, err);
    EXPECT_EQ(expectPos, state.motionH.pos);
    EXPECT_EQ(reflectDir, state.motionH.dir);
    EXPECT_EQ(beforePly.simTime + 1, state.simTime);
    EXPECT_EQ(State::HunterMotionInfo::Flag_NotStuck, state.motionH.simTimeStuck);
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
    EXPECT_EQ(beforePly.motionH.pos + beforePly.motionH.dir, state.motionH.pos);
    EXPECT_EQ(beforePly.motionH.dir, state.motionH.dir);
    EXPECT_EQ(State::HunterMotionInfo::Flag_NotStuck, state.motionH.simTimeStuck);
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
    state.motionH.dir = State::Direction(1, 1);
    state.motionH.pos = state.board.maxs;
    const State beforePly = state;
    StepH emptyStep;
    PlyError err = DoPly(emptyStep, &state);
    EXPECT_EQ(PlyError::Success, err);
    EXPECT_EQ(beforePly.motionH.pos, state.motionH.pos);
    EXPECT_EQ(-beforePly.motionH.dir, state.motionH.dir);
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
      const int wallX = state.motionH.pos.x + 1;
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
      const int wallY = state.motionH.pos.y + 1;
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
    EXPECT_EQ(beforePly.motionH.pos, state.motionH.pos);
    EXPECT_EQ(-beforePly.motionH.dir, state.motionH.dir);
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
    state.motionH.pos.x = state.board.maxs.x;
    // Place horizontal wall.
    state.walls.push_back(State::Wall());
    {
      const int wallY = state.motionH.pos.y + 1;
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
    EXPECT_EQ(beforePly.motionH.pos, state.motionH.pos);
    EXPECT_EQ(-beforePly.motionH.dir, state.motionH.dir);
    EXPECT_EQ(beforePly.simTime + 1, state.simTime);
    UndoPly(emptyStep, &state);
    EXPECT_EQ(beforePly, state);
  }
}

template <int WallDirection>
struct CreateFlagForWallType;
template <>
struct CreateFlagForWallType<State::Wall::Type_Horizontal>
{ enum { val = StepH::WallCreate_Horizontal, }; };
template <>
struct CreateFlagForWallType<State::Wall::Type_Vertical>
{ enum { val = StepH::WallCreate_Vertical, }; };

template <int WallDirection>
void TestWallCreateSingleDirection()
{
  typedef evasion::detail::WallClipHelper<WallDirection> CoordHelper;
  enum { MaxWalls = 10, };
  enum { MinCreatePeriod = 0, };
  enum { MaxCreatePeriod = 10, };
  const int numWalls = 1 + RandBound(MaxWalls);
  const int createPeriod = MinCreatePeriod + RandBound(MaxCreatePeriod - MinCreatePeriod + 1);
  State state;
  Initialize(numWalls, createPeriod, &state);
  enum { MoveType_H = 2, };
  int moveType = 0;
  StepH wallCreate;
  wallCreate.wallCreateFlag =
    static_cast<StepH::WallCreateFlag>(CreateFlagForWallType<WallDirection>::val);
  StepH emptyH;
  StepP emptyP;
  // Force P to be in an impossible location to prevent wall interference.
  state.posStackP.push_back(State::Position(BoardSizeX + HCatchesPDist,
                                            BoardSizeY + HCatchesPDist));
  for (int wallIdx = 0; wallIdx < state.maxWalls; ++wallIdx)
  {
    // Create wall. Ignore case where P is in the way (we'll try not to let
    // that happen for this test).
    const int wallFixedCoord = CoordHelper::GetFixed(state.motionH.pos);
    if (0 == (moveType % MoveType_H))
    {
      EXPECT_EQ(PlyError::Success, DoPly(wallCreate, &state));
    }
    else
    {
      EXPECT_EQ(PlyError::Success, DoPly(wallCreate, emptyP, &state));
    }
    EXPECT_EQ(state.simTime - 1, state.walls.front().simTimeCreate);
    EXPECT_EQ(0, CoordHelper::GetClip(state.walls.front().coords.p0));
    EXPECT_EQ(BoardSizeX, CoordHelper::GetClip(state.walls.front().coords.p1));
    EXPECT_EQ(CoordHelper::GetFixed(state.walls.front().coords.p0), wallFixedCoord);
    EXPECT_EQ(CoordHelper::GetFixed(state.walls.front().coords.p1), wallFixedCoord);
    ++moveType;
    // Get due time for walls.
    int wallDueTime;
    if (state.wallCreatePeriod > 1)
    {
      EXPECT_TRUE(WallCreationLockedOut(state, &wallDueTime));
    }
    else
    {
      EXPECT_FALSE(WallCreationLockedOut(state, &wallDueTime));
    }
    if (wallIdx < (numWalls - 1))
    {
      EXPECT_EQ(state.wallCreatePeriod - 2, wallDueTime);
    }
    else
    {
      EXPECT_EQ(std::numeric_limits<int>::max(), wallDueTime);
      break;
    }
    // Now wait the required time.
    for (int waitIdx = 0; waitIdx < wallDueTime; ++waitIdx)
    {
      if (0 == (moveType % MoveType_H))
      {
        EXPECT_NE(0, PlyError::WallCreationLockedOut & DoPly(wallCreate, &state));
        EXPECT_EQ(PlyError::Success, DoPly(emptyH, &state));
      }
      else
      {
        EXPECT_NE(0, PlyError::WallCreationLockedOut & DoPly(wallCreate, emptyP, &state));
        EXPECT_EQ(PlyError::Success, DoPly(emptyH, emptyP, &state));
      }
      ++moveType;
    }
  }
}
TEST(evasion_core, CreateWall)
{
  // Create horizontal walls only. All should be max coord.
  {
    TestWallCreateSingleDirection<State::Wall::Type_Horizontal>();
  }
  // Create vertical walls only. All should be max coord.
  {
    TestWallCreateSingleDirection<State::Wall::Type_Vertical>();
  }
}

}

#endif //_HPS_EVASION_EVASION_CORE_GTEST_H_
