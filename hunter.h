#ifndef _HPS_EVASION_HUNTER_
#define _HPS_EVASION_HUNTER_
#include "evasion_core.h"
#include <limits>
#include <set>
#include <iostream>

namespace hps
{
namespace evasion
{
enum Directions{North,East,West,South,NorthEast,NorthWest,SouthEast,SouthWest,Still,};
struct Hunter{
    virtual StepH Play(State* game,State::Wall* built,std::vector<State::Wall>* removed) = 0;
};

struct BasicHunter : public Hunter{
    BasicHunter(){};
    
    int FindMinHorizontalDistance(const std::vector<State::Wall>& walls, int y)
    {
        int idx = -1;
        int minDistance = std::numeric_limits<int>::max();
        for (int i = 0;i<walls.size();++i)
        {
            int distance = (walls.at(i).coords.p0.y - y);
            if(minDistance > distance)
            {
                minDistance = distance;
                idx = i;
            }
        }
        return idx;
    }
    
    int FindMinVerticalDistance(const std::vector<State::Wall>& walls, int x)
    {
        int minDistance = std::numeric_limits<int>::max();
        int idx = -1;
        for (int i = 0;i<walls.size();++i)
        {
            int distance = (walls.at(i).coords.p0.x - x);
            if(minDistance > distance)
            {
                minDistance = distance;
                idx = i;
            }
        }
        return idx;
    }
    
    StepH Play(State* game,State::Wall* built,std::vector<State::Wall>* removed)
    {
        removed->clear();
        std::cout << "inside Play..." << std::endl;
        // If the hunter is moving towards the prey in one direction
        // and is about to pass the prey (no matter how P moves)
        // then lay a wall to reduce the space available to P
        // if we can.
        
        
        State::HunterMotionInfo h = game->motionH;
        
        StepH move;
        move.wallCreateFlag = StepH::WallCreate_None;
        const State::Position& pDestination = game->posStackP.back();
        const State::Position& pSource = game->posStackP.at(game->posStackP.size()-1);
        
        
        int wallCreationAllowedIn = -1;
        bool wallCreationForbidden = WallCreationLockedOut(*game, &wallCreationAllowedIn);
        // if wall creation is not fornbidden only then compute the direction of the wall to be created.
        if( !wallCreationForbidden ) // also add the distance limit, whenever within a particular range, only then draw the wall.
        {
            ComputeWallDirection(h,pDestination,pSource,&move);
        }
        else
        {
            int idx = RandBound(game->walls.size());
            
            /*std::vector<State::Wall> horizontalWalls;
            std::vector<State::Wall> verticalWalls;
            ExtractAllHorizontal(game.walls,&horizontalWalls);
            ExtractAllVertical(game.walls,&verticalWalls);
            int horzIdx = FindMinHorizontalDistance(horizontalWalls,pDestination.y);
            int vertIdx = FindMinVerticalDistance(verticalWalls, pDestination.x);
            State::Wall wall1 = game.walls.at(horzIdx);
            State::Wall wall2 = game.walls.at(vertIdx);
            move.removeWalls.push_back(wall1);
            move.removeWalls.push_back(wall2);*/
            move.removeWalls.push_back(game->walls.at(idx));
        }
        // make a move.
        DoPly(move, game);
        State::Wall justCreatedWall = game->walls.front();
        if(justCreatedWall.simTimeCreate == game->simTime -1)
        {
            *built = justCreatedWall;
        }
        *removed = move.removeWalls;
        return move;
    }
    
    void ExtractAllHorizontal(const std::vector<State::Wall> walls, std::vector<State::Wall>* horizontalWalls)
    {
        horizontalWalls->clear();
        
        for(int i=0;i<walls.size();++i)
        {
            if(walls.at(i).type == State::Wall::Type_Horizontal)
            {
                horizontalWalls->push_back(walls.at(i));
            }
        }
    }
    
    
    void ExtractAllVertical(const std::vector<State::Wall> walls, std::vector<State::Wall>* verticalWalls)
    {
        verticalWalls->clear();
        for(int i=0;i<walls.size();++i)
        {
            if(walls.at(i).type == State::Wall::Type_Vertical)
            {
                verticalWalls->push_back(walls.at(i));
            }
        }
    }
    
    
    bool IsPreyInRange(const State::Position& hunterPos, const State::Position& preyPos)
    {
        std::cout << "checking if prey in range..." << std::endl;
        int x = hunterPos.x - preyPos.x;
        int y = hunterPos.y - preyPos.y;
        std::cout << "x: " <<x << ", y: " << y << std::endl;
        float max_distance = 10.0f;
        float dist = sqrt(x*x + y*y);
        std::cout << "max: " << max_distance << std::endl;
        std::cout << "dist: "<< dist << std::endl;
        if (max_distance < dist)
            return true;
        else
            return false;
    }
    void ComputeWallDirection(const State::HunterMotionInfo& h, 
                              const State::Position& pDestination, 
                              const State::Position& pSource, 
                              StepH* move)
    {
        std::cout << "inside wall direction..." << std::endl;
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
        Directions dirP = NorthEast;
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
