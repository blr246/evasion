#ifndef _HPS_EVASION_PREY_
#define _HPS_EVASION_PREY_
#include "evasion_core.h"
#include "rand_bound.h"
#include <vector>
#include <iostream>
namespace hps
{
namespace evasion
{

struct Prey{
  virtual StepP NextStep(State& game) = 0;
};

struct RandomPrey : public Prey{
  RandomPrey(){};

  StepP NextStep(State& state)
  {
    StepP step;
    step.moveDir.x = math::RandBound(3) - 1;
    step.moveDir.y = math::RandBound(3) - 1;
    return step;
  }
};

struct ScaredPrey : public Prey{ //Scared prey doesn't like walls (they might fall) or danger.
  StepP NextStep(State& state)
  {
    const State::Position& pPos = state.posStackP.back();
    Vector2<float> FpPos = StaticCastVector2<int, float>(pPos);
    const State::Position& hPos = state.motionH.pos;

    StepP step;

      int minWallDistance = 100000;
      Segment2<float> closestWall;
      Vector2<float> closestPoint;

      std::vector<Segment2<int> > walls;
      for(int i =  0; i < state.walls.size(); i++)
      {
        walls.push_back(state.walls[i].coords);
      }
      
      Segment2<int> fakeHunterWall;
      fakeHunterWall.p0 = hPos;
      fakeHunterWall.p1.x = hPos.x + 10000*state.motionH.dir.x;
      fakeHunterWall.p1.y = hPos.y + 10000*state.motionH.dir.y;

      walls.push_back(fakeHunterWall);

      Segment2<int> top;
      top.p0 = Vector2<int>(0,0);
      top.p1 = Vector2<int>(0,BoardSizeX);
      Segment2<int> left;
      left.p0 = Vector2<int>(0,0);
      left.p1 = Vector2<int>(BoardSizeY,0);
      Segment2<int> bottom;
      bottom.p0 = Vector2<int>(BoardSizeY,0);
      bottom.p1 = Vector2<int>(BoardSizeY,BoardSizeX);
      Segment2<int> right;
      right.p0 = Vector2<int>(0,BoardSizeX);
      right.p1 = Vector2<int>(BoardSizeY,BoardSizeX);
      walls.push_back(top);
      walls.push_back(left);
      walls.push_back(bottom);
      walls.push_back(right);

      for(int i = 0; i < walls.size(); i++)
      {
        Segment2<float> seg = StaticCastSegment<int, float>(walls[i]);
        Line<float> l1 = ExtendSegment(seg);
        Vector2<float> point = ClosestPointInLine(l1, FpPos);
        int wallDist = Vector2LengthSq(point - FpPos);
        if(wallDist < minWallDistance){
          closestWall = seg;
          minWallDistance = wallDist;
          closestPoint = point;
        }
      }

    int distX = FpPos.x - closestPoint.x;
    if(distX > 0)
    {
      step.moveDir.x = 1; // Prey is to the left of the closest wall
    }else if(distX == 0){
      step.moveDir.x = 0;
    }else{
      step.moveDir.x = -1; // Prey is to the right of the closest wall
    }
    
    int distY = FpPos.y - closestPoint.y;
    if(distY > 0)
    {
      step.moveDir.y = 1; // Prey is above the closest wall
    }else if(distY == 0){
      step.moveDir.y = 0;
    }else{
      step.moveDir.y = -1; // Prey is below the closest wall
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


struct ExtremePrey : public Prey{ //ExtremePrey likes danger.
  StepP NextStep(State& state)
  {
    const State::Position& pPos = state.posStackP.back();
    Vector2<float> FpPos = StaticCastVector2<int, float>(pPos);
    const State::Position& hPos = state.motionH.pos;

    StepP step;
      
    Line<float> HunterWall;
    HunterWall.p0 = StaticCastVector2<int, float>(hPos);
    HunterWall.dir = StaticCastVector2<int, float>(state.motionH.dir);

    Vector2<float> point = ClosestPointInLine(HunterWall, FpPos);
    int wallDist = Vector2LengthSq(point - FpPos);

    int distX = FpPos.x - point.x;
    int distY = FpPos.y - point.y;
    if(wallDist > 5*5) // Approach the death machine!
    {
      distX = -1*distX;
      distY = -1*distY;
    }

    if(distX > 0)
    {
      step.moveDir.x = 1; // Prey is to the left of the closest wall
    }else if(distX == 0){
      step.moveDir.x = 0;
    }else{
      step.moveDir.x = -1; // Prey is to the right of the closest wall
    }
    if(distY > 0)
    {
      step.moveDir.y = 1; // Prey is above the closest wall
    }else if(distY == 0){
      step.moveDir.y = 0;
    }else{
      step.moveDir.y = -1; // Prey is below the closest wall
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
