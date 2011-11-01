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
    ComputeWallDirection(h,pDestination,pSource,&move);
    
    int wallCreationAllowedIn = -1;
    bool wallCreationForbidden = WallCreationLockedOut(game, &wallCreationAllowedIn);
        
    DoPly(move, &game);
    move.wallCreateFlag = StepH::WallCreate_None;
    DoPly(move, &game);
  }
    
  void ComputeWallDirection(const State::HunterMotionInfo& h, 
                            const State::Position& pDestination, 
                            const State::Position& pSource, 
                            StepH* move)
  {
    int numerator = pDestination.y - pSource.y; // numerator in the slope of line passing through
    int denominator = pDestination.x - pSource.x; // pDestination and pSource. 
    //slope would be: numerator/denominator = (y2-y1)/(x2-x1)

    Directions dirP = GetPreyDirection(numerator,denominator);
    if(dirP != Still)
    {
        if(h.dir.x == 1 && h.dir.y == 1)
        {
            if((North == dirP || NorthEast == dirP))
            {
                move->wallCreateFlag = StepH::WallCreate_Horizontal;
            }
            else if(West == dirP || NorthWest == dirP)
            {
                move->wallCreateFlag = StepH::WallCreate_Vertical;
            }
        }
        else if(h.dir.x == -1 && h.dir.y == -1)
        {
            if((South == dirP || SouthWest == dirP))
            {
                move->wallCreateFlag = StepH::WallCreate_Horizontal;
            }
            else if(East == dirP || SouthEast == dirP)
            {
                move->wallCreateFlag = StepH::WallCreate_Vertical;
            }
        }
        else if(h.dir.x == -1 && h.dir.y == 1)
        {
            if(North == dirP || NorthWest == dirP)
            {
                move->wallCreateFlag = StepH::WallCreate_Horizontal;
            }//Horizontal
            
            else if(East == dirP || NorthEast == dirP)
            {
                move->wallCreateFlag = StepH::WallCreate_Vertical;
            }//Vertical
        }
        else if(h.dir.x ==1 && h.dir.y == -1)
        {
            if(West == dirP || NorthWest == dirP)
            {
                move->wallCreateFlag = StepH::WallCreate_Vertical;
            }// Vertical
            if(South == dirP || SouthWest == dirP)
            {
                move->wallCreateFlag = StepH::WallCreate_Horizontal;
            }
        }
    }

  }
    
  Directions GetPreyDirection(int numerator, int denominator)
  {
    Directions dirP = Still;
    // gives the direction of motion of prey.
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
    return dirP; // if none of the above is true then dirP = Still is returned.
  }

};

}
}
#endif
