#ifndef _HPS_EVASION_EVASION_CORE_H_
#define _HPS_EVASION_EVASION_CORE_H_
#include "geometry.h"
#include <vector>
#include <utility>
#include <limits>
#include <assert.h>
#include <map>
#include <sstream>
#include <iostream>

namespace hps
{
namespace evasion
{

enum { BoardSizeX = 500, };
enum { BoardSizeY = 500, };
enum { HCatchesPDist = 4, };

/// <summary> Groups the constant values for the initial position. </summary>
struct InitialPosition
{
  struct Hunter
  {
    enum { X = 0, };
    enum { Y = 0, };
  private:
    Hunter();
  };
  struct Prey
  {
    enum { X = 330, };
    enum { Y = 200, };
  private:
    Prey();
  };
private:
  InitialPosition();
};

/// <summary> The environment state. </summary>
struct State
{
  /// <summary> A board. </summary>
  typedef AxisAlignedBox2<int> Board;
  /// <summary> A position on the board. </summary>
  typedef Vector2<int> Position;
  /// <summary> A motion direction board. </summary>
  typedef Vector2<int> Direction;
  /// <summary> A wall divides the game board. </summary>
  struct Wall
  {
    /// <summary> Sort operator for walls. </summary>
    struct Sort
    {
      inline bool operator()(const Wall& lhs, const Wall& rhs) const
      {
        return lhs.simTimeCreate < rhs.simTimeCreate;
      }
    };
    /// <summary> Wall type. </summary>
    enum Type { Type_Horizontal, Type_Vertical, };
    /// <summary> Wall coordinate. </summary>
    typedef Segment2<int> Coordinates;
    inline bool operator==(const Wall& rhs) const
    {
      return (simTimeCreate == rhs.simTimeCreate) &&
             (type == rhs.type) &&
             (coords == rhs.coords);
    }
    int simTimeCreate;
    Type type;
    Coordinates coords;
  };
  /// <summary> A list of walls. </summary>
  typedef std::vector<Wall> WallList;
  /// <summary> State of the prey. </summary>
  enum PreyState { PreyState_Evading, PreyState_Captured, };
  /// <summary> Motion given by position and velocity direction. </summary>
  struct HunterMotionInfo
  {
    enum { Flag_NotStuck = 0, };
    HunterMotionInfo()
      : pos(InitialPosition::Hunter::X, InitialPosition::Hunter::Y),
        dir(1, 1),
        simTimeStuck(Flag_NotStuck)
    {}
    /// <summary> Position of hunter. </summary>
    Position pos;
    /// <summary> Hunter's unit direction of travel. </summary>
    Direction dir;
    /// <summary> Sim time when hunter became stuck. </summary>
    int simTimeStuck;
  };
  /// <summary> Position frames to pre-allocate for P. </summary>
  enum { PosStackPrealloc = 4096, };
  /// <summary> Memory of prey positions. </sumamry>
  typedef std::vector<Position> PositionStack;

  State();

  /// <summary> Value of the current time step. </summary>
  int simTime;
  /// <summary> Time of the last wall create event. </summary>
  int simTimeLastWall;
  /// <summary> Position of P. </summary>
  PositionStack posStackP;
  /// <summary> Motion of H. </summary>
  HunterMotionInfo motionH;
  /// <summary> List of walls created by H. </summary>
  WallList walls;
  /// <summary> The game board. </summary>
  Board board;
  /// <summary> Minimum time interval between wall creation for H. </summary>
  int wallCreatePeriod;
  /// <summary> Max walls created by H. </summary>
  int maxWalls;
  /// <summary> State of the prey. </sumamry>
  PreyState preyState;
  /// <summary> Temporary memory for walls manipulation. </summary>
  WallList wallsTemp;
  /// <summary> Sim time to wall idx in evasion_game_server.py </summary>
  std::map<int, int> mapSimTimeToIdx;
};

/// <summary> Data used for H to move one step. </summary>
struct StepH
{
  /// <summary> Wall create flag. <summary>
  enum WallCreateFlag
  {
    WallCreate_None,
    WallCreate_Horizontal,
    WallCreate_Vertical,
  };
  StepH() : wallCreateFlag(WallCreate_None), removeWalls() {}
  /// <summary> Desired wall creation action. </summary>
  WallCreateFlag wallCreateFlag;
  /// <summary> Walls to remove. </summary>
  State::WallList removeWalls;

  //Needs the POST-MOVE state
  std::string Serialize(State& state){
    std::stringstream ss;
    ss << "Remove:[";
    if(removeWalls.size() > 0)
    {
      for(int i = 0; i < removeWalls.size() - 1; i++)
      {
        ss << state.mapSimTimeToIdx[removeWalls[i].simTimeCreate] << ",";
      }
        ss << state.mapSimTimeToIdx[removeWalls[removeWalls.size()-1].simTimeCreate];
    }
    ss << "] Build:";

    if(wallCreateFlag == WallCreate_Horizontal)
    {
      ss << "0 ";
      ss << state.walls.back().coords.p0.x << " " << state.walls.back().coords.p1.x;
      
    }else if(wallCreateFlag == WallCreate_Vertical)
    {
      ss << "1 ";
      ss << state.walls.back().coords.p0.y << " " << state.walls.back().coords.p1.y;
    }

    ss << " ";
    return ss.str();
  }
};

/// <summary> Data used for P to move one step. </summary>
struct StepP
{
  StepP() : moveDir(0, 0) {}
  /// <summary> Direction that P will move. </summary>
  State::Direction moveDir;
  
  std::string Serialize()
  {
    std::stringstream ss;
    ss << moveDir.x << " " << moveDir.y;
    return ss.str();
  }
};

/// <summary> Bitwise or'd collection of error flags on DoPly(). </summary>
struct PlyError
{
  enum
  {
    // Success is mutually exclusive to all other flags. Check value directly.
    Success                = 0,
    // Passed a wall to remove that does not exist.
    RemoveWallNotExist     = 1 << 0,
    // Tried to create a wall during the wall creation lockout.
    WallCreationLockedOut  = 1 << 1,
    // Tried to create more walls than the maximum.
    MaxWallsExceeded       = 1 << 2,
    // Tried to create a wall but P is in the way.
    WallCreateInterference = 1 << 3,
    // The game is over.
    PreyCaptured           = 1 << 4,
  };
  PlyError() : e(Success) {}
  explicit PlyError(const int e_) : e(e_) {}
  inline operator int&()
  {
    return e;
  }
  inline operator const int&() const
  {
    return e;
  }
  int e;
};

/// <summary> Helper function to verify wall coordinates. </summary>
inline bool WallValid(const State::Wall& wall)
{
  return ((State::Wall::Type_Horizontal == wall.type) &&
          (wall.coords.p0.y == wall.coords.p1.y)) ||
         ((State::Wall::Type_Vertical == wall.type) &&
          (wall.coords.p0.x == wall.coords.p1.x));
}

/// <summary> Do a ply for H in two steps. </sumary>
/// <remarks>
///   <para> Mutate the state with the desires steps for H. H always executes
///     two steps for every one that P does. The return code flags indicate
///     errors with wall creation and destruction.
///   </para>
///   <para> On error the state is not modified. </para>
/// </remarks>
PlyError DoPly(const StepH& stepH, State* state);

/// <summary> Undo the ply. </summary>
void UndoPly(const StepH& stepH, State* state);

/// <summary> Do a ply for P. </summary>
/// <remarks>
///   <para> Returns PlyError_Success unless the prey is captured in which
///     case it returns PlyError_PlayerIncapactitated.
///   </para>
///   <para> On error the state is not modified. </para>
/// </remarks>
PlyError DoPly(const StepH& stepH, const StepP& stepP, State* state);

/// <summary> Undo the ply. </summary>
void UndoPly(const StepH& stepH, const StepP& stepP, State* state);

/// <summary> Check if the prey is captured. </summary>
inline bool PreyCaptured(const State& state)
{
  return State::PreyState_Captured == state.preyState;
}

/// <summary> Check if wall creation is locked out and return the time interval
///   until wall creation is allowed again.
/// </summary>
/// <remarks>
///   <para> For instance, suppose that wall creation is locked out with the
///     current sim time at 54. Suppose that the wall creation period is 10
///     time steps and that the last wall was created at sim time 50. Then
///     this function returns true and indicates that it is 5 time steps until
///     wall creation is allowed again since 50 + 10 - 1 = 59 and 59 - 54 - 5.
///   </para>
///   <para> If wall creation is locked out by limit of the max number of walls
///     then the time steps until it is unlocked will be infinite. If wall
///     creation has been unlocked since T time steps in the past, then the
///     return value is -T.
///   </para>
/// </remarks>
inline bool WallCreationLockedOut(const State& state, int* simTimeUnlocked)
{
  assert(simTimeUnlocked);
  // See if walls maxed out.
  if (static_cast<int>(state.walls.size()) == state.maxWalls)
  {
    *simTimeUnlocked = std::numeric_limits<int>::max();
    return true;
  }
  else
  {
    const int simTimeDue = state.simTimeLastWall + state.wallCreatePeriod;
    *simTimeUnlocked = simTimeDue - state.simTime;
    return (*simTimeUnlocked > 0);
  }
}

/// <summary> Returns true if the limit on wall creation is reached. </summary>
inline bool WallLimitReached(const State& state)
{
  return state.maxWalls == static_cast<int>(state.walls.size());
}

/// <sumamry> Initialize the state given the wall parameters. </summary>
inline void Initialize(const int maxWalls, const int wallCreatePeriod,
                       State* state)
{
  assert(state);
  assert(maxWalls >= 0);
  assert(wallCreatePeriod >= 0);
  *state = State();
  state->maxWalls = maxWalls;
  state->wallCreatePeriod = wallCreatePeriod;
  state->simTimeLastWall = -wallCreatePeriod;
}

/// <summary> Parse a string in the format given by
///   evasion_game_server.py.
/// </summary>
bool ParseStateString(const std::string& stateStr, const int m, const int n,
                      State* state);

}
using namespace evasion;
}

#endif //_HPS_EVASION_EVASION_CORE_H_
