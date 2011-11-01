#ifndef _HPS_EVASION_HUNTER_
#define _HPS_EVASION_HUNTER_
#include "evasion_core.h"
namespace hps
{
namespace evasion
{
enum Directions{North,East,West,South,NorthEast,NorthWest,SouthEast,SouthWest,Still,};
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
    move.wallCreateFlag = StepH::WallCreate_None;
    const State::Position& pDestination = game.posStackP.back();
    const State::Position& pSource = game.posStackP.at(game.posStackP.size()-1);
    
    int numerator = pDestination.y - pSource.y; // numerator in the slope of line passing through
    int denominator = pDestination.x - pSource.x; // pDestination and pSource. 
    //slope would be: numerator/denominator = (y2-y1)/(x2-x1)
    
    
    Directions dirP = GetPreyDirection(numerator,denominator);
    if(h.dir.x == 1 && h.dir.y == 1)
    {
        if((North == dirP || NorthEast == dirP))
        {
            move.wallCreateFlag = StepH::WallCreate_Horizontal;
        }
        else if(West == dirP || NorthWest == dirP)
        {
            move.wallCreateFlag = StepH::WallCreate_Vertical;
        }
    }
    else if(h.dir.x == -1 && h.dir.y == -1)
    {
        if((South == dirP || SouthWest == dirP))
        {
            move.wallCreateFlag = StepH::WallCreate_Horizontal;
        }
        else if(East == dirP || SouthEast == dirP)
        {
            move.wallCreateFlag = StepH::WallCreate_Vertical;
        }
    }
    //counter diagonal condition is missing, confirm with brandon to add that condition.
    DoPly(move, &game);
    move.wallCreateFlag = StepH::WallCreate_None;
    DoPly(move, &game);
  }
    
  Directions GetPreyDirection(int numerator, int denominator)
  {
    Directions dirP;
    // gives the direction pf motion of prey.
    if(numerator < 0 && denominator > 0 )
    {
        dirP = NorthEast;
    }
    else if(numerator > 0 && denominator < 0)
    {
        dirP = SouthWest;
    }
    else if(numerator < 0 && denominator < 0)
    {
        dirP = NorthWest;
    }
    else if(numerator > 0 && denominator > 0)
    {
        dirP = SouthWest;
    }
    else if(numerator > 0 && denominator == 0)
    {
        dirP = South;
    }
    else if(numerator < 0 && denominator == 0)
    {
        dirP = North;
    }
    else if(numerator == 0 && denominator > 0)
    {
        dirP = East;
    }
    else if(numerator == 0 && denominator < 0)
    {
        dirP = West;
    }
    else if(numerator == 0 and denominator == 0)
    {
        dirP = Still;
    }
    return dirP;
  }

};

}
}
#endif
