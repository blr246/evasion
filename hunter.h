#ifndef _HPS_EVASION_HUNTER_
#define _HPS_EVASION_HUNTER_
#include "evasion_core.h"
namespace hps
{
namespace evasion
{

struct Hunter{
  virtual void Play(State& game) = 0;
};

struct BasicHunter : public Hunter{
  BasicHunter(){};

  void Play(State& game)
  {
    // If the hunter is moving towards the prey in one direction
    // and is about to pass the prey (no matter how P moves)
    // then lay a wall to reduce the space available to P
    // if we can.
    State::HunterMotionInfo h = game.motionH;
    StepH move;
    DoPly(move, &game);
    DoPly(move, &game);
  }
};

}
}
#endif
