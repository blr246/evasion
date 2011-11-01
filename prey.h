#ifndef _HPS_EVASION_PREY_
#define _HPS_EVASION_PREY_
#include "evasion_core.h"
#include "rand_bound.h"
#include <vector>
namespace hps
{
namespace evasion
{

struct Prey{
  virtual StepP NextStep(State& game) = 0;
};

struct RandomPrey{
  RandomPrey(){};

  StepP NextStep(State& state)
  {
    StepP step;
    step.moveDir.x = math::RandBound(3) - 1;
    step.moveDir.y = math::RandBound(3) - 1;
    return step;
  }
};

struct GreedyPrey{
  StepP NextStep(State& state)
  {
    const State::Position& pPos = state.posStackP.back();
    const State::Position& hPos = state.motionH.pos;

    StepP step;

      double minWallDistance = 100000;
      Segment2<int> closestWall;
      Vector2<int> closestPoint;
      
      for(int i = 0; i < state.walls.size(); i++)
      {
        Vector2<int> point = ClosestPointInSegment(state.walls[i].coords, pPos);
        double wallDist = Vector2Length(point - pPos);
        if(wallDist < minWallDistance){
          closestWall = state.walls[i].coords;
          minWallDistance = wallDist;
          closestPoint = point;
        }
      }

      Segment2<int> fakeHunterWall;
      fakeHunterWall.p0 = hPos;
      fakeHunterWall.p1 = hPos + 1000*state.motionH.dir;
      Vector2<int> point = ClosestPointInSegment(fakeHunterWall, pPos);
      double wallDist = Vector2Length(point - pPos);
      if(wallDist < minWallDistance){
        closestWall = fakeHunterWall;
        closestPoint = point;
      }

    int distX = closestPoint.x - hPos.x;
    if(distX > 0)
    {
      step.moveDir.x = 1;
    }else{
      step.moveDir.x = -1;
    }
    
    int distY = closestPoint.y - hPos.y;
    if(distY > 0)
    {
      step.moveDir.y = 1;
    }else{
      step.moveDir.y = -1;
    }

    distX = pPos.x - hPos.x;

    if(abs(distX) == 1){
      step.moveDir.x = -1 * distX;
    }

    distY = pPos.y - hPos.y;
    if(abs(distY) == 1){
      step.moveDir.y = -1 * distY;
    }

    return step;
  }
};

}
}
#endif
