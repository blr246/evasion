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
  // reissb -- 20111101 -- Have to sort walls before we compare.
  State::WallList lhsWallsSorted = lhs.walls;
  std::sort_heap(lhsWallsSorted.begin(), lhsWallsSorted.end(), State::Wall::Sort());
  State::WallList rhsWallsSorted = rhs.walls;
  std::sort_heap(rhsWallsSorted.begin(), rhsWallsSorted.end(), State::Wall::Sort());
  return (lhs.simTime == rhs.simTime) &&
         (lhs.simTimeLastWall == rhs.simTimeLastWall) &&
         (lhs.posStackP == rhs.posStackP) &&
         (lhs.motionH == rhs.motionH) &&
         (lhsWallsSorted == rhsWallsSorted) &&
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

TEST(evasion_core, ReadState)
{
  // The following were recovered from the console.
  // ***
  // You are Hunter
  // Hunter:0 0 1 1
  // Prey:  330 200
  // Walls: []
  // 
  // ***
  // You are Prey
  // Hunter:0 0 1 1
  // Prey:  330 200
  // Walls: []
  // 
  // ***
  {
    const char stateStr[] = "You are Hunter\n"
                            "Hunter:0 0 1 1\n"
                            "Prey:  330 200\n"
                            "Walls: []\n"
                            "\n";
    State state;
    EXPECT_TRUE(ParseStateString(std::string(stateStr), 3, 4, &state));
  }
  {
    const char stateStr[] = "You are Prey\n"
                            "Hunter:0 0 1 1\n"
                            "Prey:  330 200\n"
                            "Walls: []\n"
                            "\n";
    State state;
    EXPECT_TRUE(ParseStateString(std::string(stateStr), 3, 4, &state));
  }
  // Walls: [wall_index (x0, y0, x1, y1), wall_index (x0, y0, x1, y1), ...]
  {
    const char stateStr[] = "You are Prey\n"
                            "Hunter:0 0 1 1\n"
                            "Prey:  330 200\n"
                            "Walls: [0 (0, 0, 0, 499), 1 (1, 4, 499, 4)]\n"
                            "\n";
    State state;
    EXPECT_TRUE(ParseStateString(std::string(stateStr), 3, 4, &state));
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
                                 Pos(BoardSizeX - 1, BoardSizeY / 2));
        PlyHSingleWallTestAllDir(wall, false);
      }
      {
        wall.type = State::Wall::Type_Vertical;
        wall.coords = WallCoords(Pos(BoardSizeX / 2, 0),
                                 Pos(BoardSizeX / 2, BoardSizeY - 1));
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

template <int Dir0>
void TestWallCreate(const bool alternateDir)
{
  typedef evasion::detail::WallClipHelper<Dir0> CoordHelper0;
  enum { Dir1 = CoordHelper0::OtherDirection, };
  typedef evasion::detail::WallClipHelper<Dir1> CoordHelper1;
  enum { MaxWalls = 10, };
  enum { MinCreatePeriod = 1, };
  enum { MaxCreatePeriod = 10, };
  const int numWalls = 1 + RandBound(MaxWalls);
  const int createPeriod = MinCreatePeriod + RandBound(MaxCreatePeriod - MinCreatePeriod + 1);
  State state;
  Initialize(numWalls, createPeriod, &state);
  enum { MoveType_H = 2, };
  int moveType = 0;
  StepH wallCreate0;
  wallCreate0.wallCreateFlag =
    static_cast<StepH::WallCreateFlag>(CreateFlagForWallType<Dir0>::val);
  StepH wallCreate1;
  wallCreate1.wallCreateFlag =
    static_cast<StepH::WallCreateFlag>(CreateFlagForWallType<Dir1>::val);
  StepH emptyH;
  StepP emptyP;
  // Force P to be in an impossible location to prevent wall interference.
  state.posStackP.push_back(State::Position(BoardSizeX + HCatchesPDist,
                                            BoardSizeY + HCatchesPDist));
  State::Position wallCreatePosLast(-1, -1);
  for (int wallIdx = 0; wallIdx < state.maxWalls; ++wallIdx)
  {
    EXPECT_EQ(moveType, state.simTime);
    // Create wall. Ignore case where P is in the way (we'll try not to let
    // that happen for this test).
    const int alternateId = alternateDir && (wallIdx % 2);
    const StepH& wallCreate = alternateId ? wallCreate1 : wallCreate0;
    const int wallFixedCoord = (alternateId) ?
                               CoordHelper1::GetFixed(state.motionH.pos) :
                               CoordHelper0::GetFixed(state.motionH.pos);
    State::Position wallCreatePosNow = state.motionH.pos;
    if (0 == (moveType % MoveType_H))
    {
      const State stateBefore = state;
      EXPECT_EQ(PlyError::Success, DoPly(wallCreate, &state));
      UndoPly(wallCreate, &state);
      EXPECT_EQ(stateBefore, state);
      DoPly(wallCreate, &state);
    }
    else
    {
      const State stateBefore = state;
      EXPECT_EQ(PlyError::Success, DoPly(wallCreate, emptyP, &state));
      UndoPly(wallCreate, emptyP, &state);
      EXPECT_EQ(stateBefore, state);
      DoPly(wallCreate, emptyP, &state);
    }
    EXPECT_EQ(state.simTime - 1, state.walls.front().simTimeCreate);
    // Clip coords when alternating.
    if (alternateDir)
    {
      // Fixed coordinate is from H position at wall creation time.
      // Clip coordinat is the previous wall plus one. The previous wall was
      // created at the last created time.
      if (alternateId)
      {
        typedef CoordHelper1 CoordHelper;
        assert(&wallCreate == &wallCreate1);
        // Fixed.
        EXPECT_EQ(CoordHelper::GetFixed(wallCreatePosNow),
                  CoordHelper::GetFixed(state.walls.front().coords.p0));
        EXPECT_EQ(CoordHelper::GetFixed(wallCreatePosNow),
                  CoordHelper::GetFixed(state.walls.front().coords.p1));
        // Clip.
        EXPECT_EQ(CoordHelper::GetClip(wallCreatePosLast) + 1,
                  CoordHelper::GetClip(state.walls.front().coords.p0));
        EXPECT_EQ(CoordHelper::GetClip(state.board.maxs),
                  CoordHelper::GetClip(state.walls.front().coords.p1));
      }
      else
      {
        assert(&wallCreate == &wallCreate0);
        typedef CoordHelper0 CoordHelper;
        // Fixed.
        EXPECT_EQ(CoordHelper::GetFixed(wallCreatePosNow),
                  CoordHelper::GetFixed(state.walls.front().coords.p0));
        EXPECT_EQ(CoordHelper::GetFixed(wallCreatePosNow),
                  CoordHelper::GetFixed(state.walls.front().coords.p1));
        // Clip.
        EXPECT_EQ(CoordHelper::GetClip(wallCreatePosLast) + 1,
                  CoordHelper::GetClip(state.walls.front().coords.p0));
        EXPECT_EQ(CoordHelper::GetClip(state.board.maxs),
                  CoordHelper::GetClip(state.walls.front().coords.p1));
      }
    }
    else
    {
      assert(&wallCreate == &wallCreate0);
      typedef CoordHelper0 CoordHelper;
      // Fixed.
      EXPECT_EQ(wallFixedCoord, CoordHelper::GetFixed(state.walls.front().coords.p0));
      EXPECT_EQ(wallFixedCoord, CoordHelper::GetFixed(state.walls.front().coords.p1));
      // Clip.
      EXPECT_EQ(CoordHelper::GetClip(state.board.mins),
                CoordHelper::GetClip(state.walls.front().coords.p0));
      EXPECT_EQ(CoordHelper::GetClip(state.board.maxs),
                CoordHelper::GetClip(state.walls.front().coords.p1));
    }
    wallCreatePosLast = wallCreatePosNow;
    ++moveType;
    // Leave when walls maxed.
    if (wallIdx == (numWalls - 1))
    {
      assert(static_cast<int>(state.walls.size()) == numWalls);
      break;
    }
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
    EXPECT_EQ(state.wallCreatePeriod - 1, wallDueTime);
    // Now wait the required time.
    for (int waitIdx = 0; waitIdx < wallDueTime; ++waitIdx)
    {
      if (0 == (moveType % MoveType_H))
      {
        EXPECT_NE(0, PlyError::WallCreationLockedOut & DoPly(wallCreate, &state));
        const State stateBefore = state;
        EXPECT_EQ(PlyError::Success, DoPly(emptyH, &state));
        UndoPly(emptyH, &state);
        EXPECT_EQ(stateBefore, state);
        DoPly(emptyH, &state);
      }
      else
      {
        EXPECT_NE(0, PlyError::WallCreationLockedOut & DoPly(wallCreate, emptyP, &state));
        const State stateBefore = state;
        EXPECT_EQ(PlyError::Success, DoPly(emptyH, emptyP, &state));
        UndoPly(emptyH, emptyP, &state);
        EXPECT_EQ(stateBefore, state);
        DoPly(emptyH, emptyP, &state);
      }
      ++moveType;
    }
  }
  int wallDueTime;
  EXPECT_TRUE(WallCreationLockedOut(state, &wallDueTime));
  EXPECT_EQ(std::numeric_limits<int>::max(), wallDueTime);
}
TEST(evasion_core, CreateWall)
{
#ifdef NDEBUG
  enum { CreateWallIterations = 10, };
#else
  enum { CreateWallIterations = 5, };
#endif
  // Create horizontal walls only. All should be max coord.
  {
    SCOPED_TRACE("Wall create - horizontal");
    for (int iteration = 0; iteration < CreateWallIterations; ++iteration)
    {
      TestWallCreate<State::Wall::Type_Horizontal>(false);
    }
  }
  // Create vertical walls only. All should be max coord.
  {
    SCOPED_TRACE("Wall create - vertical");
    for (int iteration = 0; iteration < CreateWallIterations; ++iteration)
    {
      TestWallCreate<State::Wall::Type_Vertical>(false);
    }
  }
  // Test alternating directions.
  {
    SCOPED_TRACE("Wall create - alternating starting with horizontal");
    for (int iteration = 0; iteration < CreateWallIterations; ++iteration)
    {
      TestWallCreate<State::Wall::Type_Horizontal>(true);
    }
  }
  // Test alternating directions.
  {
    SCOPED_TRACE("Wall create - alternating starting with vertical");
    for (int iteration = 0; iteration < CreateWallIterations; ++iteration)
    {
      TestWallCreate<State::Wall::Type_Vertical>(true);
    }
  }
}

TEST(evasion_core, RemoveWalls)
{
  // Create some walls, then remove a random number [1, N] and make sure that
  // the state is reverisble.
  State state;
  Initialize(3, 3, &state);

  enum { Iterations = 1000, };
  enum { MoveType_H = 2, };
  int moveType = MoveType_H;
  // Force P to be in an impossible location to prevent wall interference.
  state.posStackP.push_back(State::Position(BoardSizeX + HCatchesPDist,
                                            BoardSizeY + HCatchesPDist));
  // Do limited number of iterations.
  while (moveType < Iterations)
  {
    int dueTime;
    const bool makeWallLocked = (WallCreationLockedOut(state, &dueTime));
    const bool noWalls = state.walls.empty();
    enum { NWallMkProb = 50, };
    enum { NWallRmProb = 200, };
    // Make walls exclusively from removing them in this test.
    const bool makeWall = !makeWallLocked && (0 == RandBound(NWallMkProb));
    const bool rmWall = !noWalls && !makeWall && (0 == RandBound(NWallRmProb));
    // First setup the move.
    StepH stepH;
    if (rmWall)
    {
      assert(!makeWall);
      // Copy walls and random shuffle.
      State::WallList walls = state.walls;
      std::random_shuffle(walls.begin(), walls.end());
      // Remove random wall(s).
      const int rmCount = RandBound(walls.size()) + 1;
      stepH.removeWalls.resize(rmCount);
      std::copy(walls.begin(), walls.begin() + rmCount,
                stepH.removeWalls.begin());
    }
    else
    {
      // Either make a wall or do nothing.
      assert((makeWall && !rmWall) || (!makeWall && !rmWall));
      if (makeWall)
      {
        const int wallType = RandBound(2);
        if (0 == wallType)
        {
          stepH.wallCreateFlag = StepH::WallCreate_Horizontal;
        }
        else
        {
          stepH.wallCreateFlag = StepH::WallCreate_Vertical;
        }
      }
    }
    // Alternate between {H} and {H, P} moves.
    if (0 == (moveType % MoveType_H))
    {
      if (rmWall)
      {
        // Remove the walls and make sure that the move is reversible.
        const State stateBefore = state;
        EXPECT_EQ(PlyError::Success, DoPly(stepH, &state));
        UndoPly(stepH, &state);
        EXPECT_EQ(stateBefore, state);
        DoPly(stepH, &state);
      }
      else
      {
        // Making walls is tested separately. Just do it.
        EXPECT_EQ(PlyError::Success, DoPly(stepH, &state));
      }
    }
    else
    {
      StepP stepP;
      stepP.moveDir = State::Direction(RandBound(3) - 1, RandBound(3) - 1);
      if (rmWall)
      {
        // Remove the walls and make sure that the move is reversible.
        const State stateBefore = state;
        EXPECT_EQ(PlyError::Success, DoPly(stepH, stepP, &state));
        UndoPly(stepH, stepP, &state);
        EXPECT_EQ(stateBefore, state);
        DoPly(stepH, stepP, &state);
      }
      else
      {
        // Making walls is tested separately. Just do it.
        EXPECT_EQ(PlyError::Success, DoPly(stepH, stepP, &state));
      }
    }
    ++moveType;
  }
}

}

#endif //_HPS_EVASION_EVASION_CORE_GTEST_H_
