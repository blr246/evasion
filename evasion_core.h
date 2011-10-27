#ifndef _HPS_EVASION_EVASION_CORE_H_
#define _HPS_EVASION_EVASION_CORE_H_
#include "geometry.h"
#include <vector>

namespace hps
{
namespace evasion
{

enum { BoardSizeX = 500, };
enum { BoardSizeY = 500, };

/// <summary> Groups the constant values for the Prey initial position. </summary>
struct PInitialPosition
{
  enum { X = 330, };
  enum { Y = 200, };
private:
  PInitialPosition();
};
/// <summary> Groups the constant values for the Hunter initial position. </summary>
struct HInitialPosition
{
  enum { X = 0, };
  enum { Y = 0, };
private:
  HInitialPosition();
};

/// <summary> The environment state. </summary>
struct State
{
  typedef Vector2<int> Position;
  typedef Vector2<int> Direction;
  typedef Segment2<int> Wall;
  typedef AxisAlignedBox<int> Board;
  typedef std::vector<Wall> WallList;

  struct MotionInfo
  {
    MotionInfo();
    MotionInfo(const Position& s_, const Direction& d_);

    /// <summary> Position s(t) as in physics. </summary>
    Position s;
    /// <summary> Unit direction of travel. </summary>
    Direction d;
  };

  State();

  /// <summary> Value of the current time step. </summary>
  int simTime;
  /// <summary> Motion of P. </summary>
  MotionInfo motionP;
  /// <summary> Motion of H. </summary>
  MotionInfo motionH;
  /// <summary> List of walls created by H. </summary>
  WallList walls;
  /// <summary> The game board. </summary>
  Board board;
  /// <summary> Minimum time interval between wall creation for H. </summary>
  int wallCreatePeriod;
  /// <summary> Max walls created by H. </summary>
  int maxWalls;
};

}
using namespace evasion;
}

#endif //_HPS_EVASION_EVASION_CORE_H_
