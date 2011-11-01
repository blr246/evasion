#ifndef _HPS_EVASION_EVASION_GAME_GTEST_H_
#define _HPS_EVASION_EVASION_GAME_GTEST_H_
#include "evasion_core.h"
#include "process.h"
#include "prey.h"
#include "hunter.h"
#include <vector>
#include <string>
#include <sstream>

namespace _hps_evasion_evasion_game_gtest_h_
{
using namespace hps;

/// <summary> Initialize vis with the game state. </summary>
inline void InitializeVis(const State& state, Process* vis)
{
  assert(vis);
  std::vector<std::string> visArgs;
  {
    visArgs.push_back(std::string("python"));
    visArgs.push_back(std::string("evasion_vis.py"));
  }
  EXPECT_TRUE(vis->Start(visArgs));
  // Send the vis process configuration.
  std::stringstream ssDefaultGameConfig;
  ssDefaultGameConfig << "<EvasionVis.SetupParams>"
                      <<   "<DimX>" << state.board.maxs.x << "</DimX>"
                      <<   "<DimY>" << state.board.maxs.y << "</DimY>"
                      <<   "<MaxWalls>" << state.maxWalls << "</MaxWalls>"
                      <<   "<WallMakePeriod>" << state.wallCreatePeriod
                      <<   "</WallMakePeriod>"
                      << "</EvasionVis.SetupParams>" << std::endl;
  vis->WriteStdin(ssDefaultGameConfig.str());
}

/// <summary> Update the visualization with the current game state. </summary>
inline void UpdateVis(const State& state, Process* vis)
{
  assert(vis);
  std::stringstream ssUpdate;
  const State::Position posP = state.posStackP.back();
  const State::Position posH = state.motionH.pos;
  ssUpdate
    << "<EvasionVis.GameUpdate>"
    <<   "<PosP>(" << posP.x << ", " << posP.y << ")</PosP>"
    <<   "<PosH>(" << posH.x << ", " << posH.y << ")</PosH>"
    <<   "<Walls>[";
  int wallIdx = 0;
  for (State::WallList::const_iterator wall = state.walls.begin();
       wall != state.walls.end();
       ++wall, ++wallIdx)
  {
    if (wallIdx > 0)
    {
      ssUpdate << ", ";
    }
    ssUpdate << "((" << wall->coords.p0.x << ", " << wall->coords.p0.y << "), "
             <<  "(" << wall->coords.p1.x << ", " << wall->coords.p1.y << "))";
  }
  ssUpdate <<   "]</Walls>"
           << "</EvasionVis.GameUpdate>" << std::endl;
  vis->WriteStdin(ssUpdate.str());
}

TEST(evasion_game, DemoVisTest)
{
  // Game state.
  State state;

  // Create a vis process.
  Process vis;
  InitializeVis(state, &vis);

  // Send vis a new state. This is where you play the game.
  UpdateVis(state, &vis);

  // Vis will quit when it receives an empty line.
  vis.WriteStdin(std::string("\r\n"));
  vis.Join();
}

void TestPrey(Prey& prey, int MaxIterations)
{
  // Game state.
  State state;
  Initialize(3, 3, &state);

  // Create a vis process.
  Process vis;
  InitializeVis(state, &vis);

  enum { MoveType_H = 2, };
  int moveType = MoveType_H;
  // We will limit iterations here for demo purposes.
  // Do limited number of iterations.
  while (moveType < MaxIterations)
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
    PlyError err;
    // Alternate between {H} and {H, P} moves.
    if (0 == (moveType % MoveType_H))
    {
      err = DoPly(stepH, &state);
    }
    else
    {
      StepP stepP = prey.NextStep(state);
      err = DoPly(stepH, stepP, &state);
    }
    // Do-over on case when the move does not succeed. This happens rarely when
    // H is trapped in a minutely confined space.
    if (PlyError::Success != err)
    {
      continue;
    }
    UpdateVis(state, &vis);
    ++moveType;
  }

  // Vis will quit when it receives an empty line.
  vis.WriteStdin(std::string("\r\n"));
  vis.Join();

  if (PreyCaptured(state))
  {
    std::cout << "Prey captured at time " << state.simTime << "." << std::endl;
  }
  else
  {
    std::cout << "Timed out at time " << state.simTime << "." << std::endl;
  }
}
TEST(evasion_game, RandomPreyRandomHunter)
{
  RandomPrey p;
  enum { MaxIterations = 100, };
  TestPrey(p, MaxIterations);
}

TEST(evasion_game, GreedyPreyRandomHunter)
{
  ScaredPrey p;
  enum { MaxIterations = 100, };
  TestPrey(p, MaxIterations);
}

TEST(evasion_game, ExtremePreyRandomHunter)
{
  ExtremePrey p;
  enum { MaxIterations = 100, };
  TestPrey(p, MaxIterations);
}

TEST(BasicHunter, Play)
{
    // Game state.
    State state;
    Initialize(3, 3, &state);
    BasicHunter hunter;
    RandomPrey prey;
    
    // Create a vis process.
    Process vis;
    InitializeVis(state, &vis);
    
    enum { MaxIterations = 10000, };
    enum { MoveType_H = 2, };
    int moveType = MoveType_H;
    // We will limit iterations here for demo purposes.
    // Do limited number of iterations.
    while (moveType < MaxIterations)
    {
        State::Wall built;
        std::vector<State::Wall> removed;
        std::cout << "calling Play..." <<std::endl;
        StepH stepH = hunter.Play(state, &built, &removed);
        stepH.wallCreateFlag = StepH::WallCreate_None;
        DoPly(stepH, &state);
        StepP stepP = prey.NextStep(state);
        DoPly(stepH, stepP, &state);
        
        UpdateVis(state, &vis);
        ++moveType;
    }
    
    // Vis will quit when it receives an empty line.
    vis.WriteStdin(std::string("\r\n"));
    vis.Join();
    
    if (PreyCaptured(state))
    {
        std::cout << "Prey captured at time " << state.simTime << "." << std::endl;
    }
    else
    {
        std::cout << "Timed out at time " << state.simTime << "." << std::endl;
    }
}

}

#endif //_HPS_EVASION_EVASION_GAME_GTEST_H_
