#ifndef _HPS_EVASION_EVASION_GAME_GTEST_H_
#define _HPS_EVASION_EVASION_GAME_GTEST_H_
#include "evasion_core.h"
#include "process.h"
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

TEST(evasion_game, RandomStrategy)
{
  // Game state.
  State state;
  Initialize(3, 3, &state);

  // Create a vis process.
  Process vis;
  InitializeVis(state, &vis);

  // Send vis a new state. This is where you play the game.
  enum { MoveType_H = 2, };
  int moveType = MoveType_H;
  // The Hunter cannot win. We will limit iterations here for demo purposes.
  while (!PreyCaptured(state))
  {
    if (0 == (moveType % MoveType_H))
    {
      StepH stepH;
      int dueTime;
      if (WallCreationLockedOut(state, &dueTime))
      {
        // 1 in N shot of removing a wall.
        enum { NWallRmProb = 200, };
        const int rmWalls = RandBound(NWallRmProb);
        if (0 == rmWalls)
        {
          State::WallList walls = state.walls;
          const int rmCount = RandBound(walls.size()) + 1;
          std::random_shuffle(walls.begin(), walls.end());
          // Remove random wall(s).
          stepH.removeWalls.resize(rmCount);
          std::copy(walls.begin(), walls.begin() + rmCount,
                    stepH.removeWalls.begin());
        }
      }
      else
      {
        // 1 in N shot of making a wall.
        enum { NWallMkProb = 50, };
        const int makeWall = RandBound(NWallMkProb);
        if (0 == makeWall)
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
      if (PlyError::Success != DoPly(stepH, &state))
      {
        continue;
      }
    }
    else
    {
      StepH stepH;
      StepP stepP;
      stepP.moveDir = State::Direction(RandBound(3) - 1, RandBound(3) - 1);
      DoPly(stepH, stepP, &state);
    }
    UpdateVis(state, &vis);
    ++moveType;
  }

  // Vis will quit when it receives an empty line.
  vis.WriteStdin(std::string("\r\n"));
  vis.Join();

  std::cout << "Prey captured at time " << state.simTime << "." << std::endl;
}

}

#endif //_HPS_EVASION_EVASION_GAME_GTEST_H_
