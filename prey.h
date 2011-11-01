#ifndef _HPS_EVASION_PREY_
#define _HPS_EVASION_PREY_
#include "evasion_core.h"
#include "rand_bound.h"
namespace hps
{
namespace evasion
{

struct Prey{
  virtual StepP NextStep(State& game) = 0;
};

struct RandomPrey{
  RandomPrey(){};

  StepP NextStep(State& game)
  {
    StepP step;
    step.moveDir.x = math::RandBound(3) - 1;
    step.moveDir.y = math::RandBound(3) - 1;
    return step;
  }
};

struct GreedyPrey{
  StepP NextStep(State& game)
  {
    const State::Position& pPos = game.posStackP.back();
    const State::Position& hPos = game.motionH.pos;

    StepP step;
    //Set X direction:
    int distX = pPos.x - hPos.x;

    if(abs(distX) == 1){
      step.moveDir.x = -1 * distX;
    }
    
    //Set Y direction:
    int distY = pPos.y - hPos.y;

    if(abs(distY) == 1){
      step.moveDir.y = -1 * distY;
    }
    
    return step;
  }
};

}
}
#endif
