#include "evasion_core.h"
#include <sstream>
#include <iostream>
#include <algorithm>

namespace hps
{
namespace evasion
{

namespace detail
{
template <char Delimiter>
inline void StreamFlagsPrependDelim(const int flagCount, std::ostream* strm)
{
  assert(strm);
  if (flagCount > 0)
  {
    *strm << Delimiter;
  }
}
}

inline std::ostream& operator<<(std::ostream& strm, const PlyError err)
{

  // Success does not occur with any other flag.
  if (PlyError::Success == err)
  {
    strm << "Success";
  }
  // Stream all other flags.
  else
  {
    std::stringstream ssErr;
    int flags = 0;
    if (PlyError::RemoveWallNotExist & err)
    {
      detail::StreamFlagsPrependDelim<'|'>(flags, &strm);
      ssErr << "RemoveWallNotExist";
    }
    if (PlyError::WallCreationLockedOut & err)
    {
      detail::StreamFlagsPrependDelim<'|'>(flags, &strm);
      ssErr << "WallCreationLockedOut";
    }
    if (PlyError::MaxWallsExceeded & err)
    {
      detail::StreamFlagsPrependDelim<'|'>(flags, &strm);
      ssErr << "MaxWallsExceeded";
    }
    if (PlyError::WallCreateInterference & err)
    {
      detail::StreamFlagsPrependDelim<'|'>(flags, &strm);
      ssErr << "WallCreateInterference";
    }
    if (PlyError::PlayerIncapacitated & err)
    {
      detail::StreamFlagsPrependDelim<'|'>(flags, &strm);
      ssErr << "PlayerIncapacitated";
    }
    strm << ssErr.str();
  }
  return strm;
}

State::MotionInfo::MotionInfo() : s(), d() {}

State::MotionInfo::MotionInfo(const Position& s_, const Direction& d_)
: s(s_),
  d(d_)
{}

State::State()
: simTime(0),
  simTimeLastWall(0),
  motionP(Position(PInitialPosition::X, PInitialPosition::Y), Direction(0, 0)),
  motionH(Position(HInitialPosition::X, HInitialPosition::Y), Direction(1, 1)),
  walls(),
  board(Vector2<int>(0, 0), Vector2<int>(BoardSizeX, BoardSizeY)),
  wallCreatePeriod(0),
  maxWalls(0),
  preyCaptureInfo(std::make_pair(Prey_Evading, 0))
{}

namespace detail
{
template <int WallDirection>
struct WallClipHelper
{
  typedef State::Position Position;
  typedef State::Wall::Coordinates WallCoordinates;
  typedef State::Wall Wall;
  typedef State::WallList WallList;

  template <int WallDirection>
  struct CoordHelper;

  /// <summary> Coordinate helper for horizontal wall clipping. </summary>
  template <>
  struct CoordHelper<State::Wall::Type_Horizontal>
  {
    enum { OtherDirection = State::Wall::Type_Vertical, };
    inline static const int& GetFixed(const Position& pt) { return pt.y; }
    inline static int& GetFixed(Position& pt) { return pt.y; }
    inline static const int& GetClip(const Position& pt) { return pt.x; }
    inline static int& GetClip(Position& pt) { return pt.x; }
    inline static const WallCoordinates InitWallCoords(const int fixed)
    {
      return WallCoordinates(Position(0, fixed), Position(BoardSizeX, fixed));
    }
  };

  /// <summary> Coordinate helper for vertical wall clipping. </summary>
  template <>
  struct CoordHelper<State::Wall::Type_Vertical>
  {
    enum { OtherDirection = State::Wall::Type_Horizontal, };
    inline static const int& GetFixed(const Position& pt) { return pt.x; }
    inline static int& GetFixed(Position& pt) { return pt.x; }
    inline static const int& GetClip(const Position& pt) { return pt.y; }
    inline static int& GetClip(Position& pt) { return pt.y; }
    inline static const WallCoordinates InitWallCoords(const int fixed)
    {
      return WallCoordinates(Position(fixed, 0), Position(fixed, BoardSizeY));
    }
  };

  /// <summary> Create a wall clipped by a list of existing walls. </summary>
  static Wall CreateClipped(const Position& pt, const WallList& walls)
  {
    typedef CoordHelper<WallDirection> CoordHelp;
    const int fixed = CoordHelp::GetFixed(pt);
    Wall wall;
    {
      wall.type = static_cast<State::Wall::Type>(WallDirection);
      wall.coords = CoordHelp::InitWallCoords(fixed);
    }
    WallCoordinates& wallCoords = wall.coords;
    // Clip wall using all perpendicular walls.
    for (WallList::const_iterator clipW = walls.begin();
         clipW != walls.end();
         ++clipW)
    {
      const WallCoordinates& clipCoords = clipW->coords;
      if ((CoordHelp::OtherDirection == clipW->type) &&
          ((CoordHelp::GetFixed(clipCoords.p0) <= fixed) &&
           (CoordHelp::GetFixed(clipCoords.p1) >= fixed)))
      {
        // Clip wall toward H.
        assert(CoordHelp::GetClip(clipCoords.p0) ==
               CoordHelp::GetClip(clipCoords.p1));
        const int clip = CoordHelp::GetClip(clipCoords.p0);
        int& clipped = CoordHelp::GetClip(wallCoords.p0);
        if (clip <= CoordHelp::GetClip(pt))
        {
          clipped = std::max(clip, clipped);
        }
        else
        {
          clipped = std::min(clip, clipped);
        }
      }
    }
    return wall;
  }

  /// <summary> See if the point collides with the wall. </sumamry>
  inline static bool CheckCollision(const WallCoordinates& wc,
                                    const Position& pt)
  {
    typedef CoordHelper<WallDirection> CoordHelp;
    assert(CoordHelp::GetFixed(wc.p0) == CoordHelp::GetFixed(wc.p1));
    return (CoordHelp::GetFixed(pt) == CoordHelp::GetFixed(wc.p0)) &&
           (CoordHelp::GetClip(wc.p0) <= CoordHelp::GetClip(pt)) &&
           (CoordHelp::GetClip(wc.p1) >= CoordHelp::GetClip(pt));
  }
};
/// <summary> Advance the player's motion. </summary>
/// <remarks>
///   <para> Apply the motion info direction to the position. In the case of a
///     player that can move freely, the direction field must be updated for
///     the curren time step prior to applying this function.
///   </para>
/// </remarks>
void MovePlayer(const State& state, State::MotionInfo* motion)
{
  assert(motion);
  // Nothing to do when player is stationary.
  static const State::Direction moveZero(0, 0);
  if (moveZero == motion->d)
  {
    return;
  }
  const State::Position posNew = motion->s + motion->d;
  bool hitWallH = false;
  bool hitWallV = false;
  // Check collision with board.
  const State::Board& board = state.board;
  if ((posNew.y < board.mins.y) || (board.maxs.y < posNew.y))
  {
    hitWallH = true;
  }
  if ((posNew.x < board.mins.x) || (board.maxs.x < posNew.x))
  {
    hitWallV = true;
  }
  // Check collision with all walls.
  const State::WallList& walls = state.walls;
  for (State::WallList::const_iterator wall = walls.begin();
       wall != walls.end();
       ++wall)
  {
    if (State::Wall::Type_Horizontal == wall->type)
    {
      typedef WallClipHelper<State::Wall::Type_Horizontal> CollisionHelper;
      hitWallH |= CollisionHelper::CheckCollision(wall->coords, posNew);
    }
    else
    {
      assert(State::Wall::Type_Vertical == wall->type);
      typedef WallClipHelper<State::Wall::Type_Vertical> CollisionHelper;
      hitWallV |= CollisionHelper::CheckCollision(wall->coords, posNew);
    }
  }
  // Perform reflection. Bounce off of wall type.
  if (hitWallH)
  {
    motion->d.y *= -1;
  }
  if (hitWallV)
  {
    motion->d.x *= -1;
  }
  // Move player.
  motion->s += motion->d;
}
/// <summary> Internal helper function to apply an H step. </summary>
PlyError DoStepH(const StepH& step, State* state)
{
  assert(state);
  PlyError err(PlyError::Success);
  State::WallList& walls = state->walls;
  State::Wall wall;
  wall.simTimeCreate = state->simTime;
  const int numWalls = static_cast<int>(walls.size());
  // See if wall creation requested and allowed.
  const bool wantCreateWall = StepH::WallCreate_None != step.wallCreateFlag;
  if (wantCreateWall)
  {
    // See if wall creation is legal.
    int simTimeUntilWallCreate;
    if (WallCreationLockedOut(*state, &simTimeUntilWallCreate))
    {
      std::cerr << "DoStepH() : "
                << "Wall creation attempted during lockout." << std::endl;
      err |= PlyError::WallCreationLockedOut;
    }
    else
    {
      // See if wall will intersect P by first creating the wall.
      const State::Position& posH = state->motionH.s;
      const State::Position& posP = state->motionP.s;
      if (StepH::WallCreate_Horizontal == step.wallCreateFlag)
      {
        typedef WallClipHelper<State::Wall::Type_Horizontal> WallClipCreator;
        assert(StepH::WallCreate_Horizontal == step.wallCreateFlag);
        wall = WallClipCreator::CreateClipped(posH, walls);
        assert(State::Wall::Type_Horizontal == wall.type);
        // Check interference.
        if (WallClipCreator::CheckCollision(wall.coords, posP))
        {
          std::cerr << "DoStepH() : Creation of horizontal wall cannot proceed "
                    << "since P is in the way." << std::endl;
          err |= PlyError::WallCreateInterference;
        }
      }
      else
      {
        typedef WallClipHelper<State::Wall::Type_Vertical> WallClipCreator;
        assert(StepH::WallCreate_Vertical == step.wallCreateFlag);
        wall = WallClipCreator::CreateClipped(posH, walls);
        assert(State::Wall::Type_Vertical == wall.type);
        // Check interference.
        if (WallClipCreator::CheckCollision(wall.coords, posP))
        {
          std::cerr << "DoStepH() : Creation of vertical wall cannot proceed "
                    << "since P is in the way." << std::endl;
          err |= PlyError::WallCreateInterference;
        }
      }
    }
  }
  // See if wall removals are valid. Use walls as temporary storage.
  const bool wantRemoveWall = !step.removeWalls.empty();
  if (wantRemoveWall)
  {
    // Copy walls to itself for temporary storage.
    typedef State::WallList::iterator WallRemovePos;
    walls.resize(2 * numWalls);
    WallRemovePos wallsCopyBegin = walls.begin() + numWalls;
    std::copy(walls.begin(), wallsCopyBegin, wallsCopyBegin);
    // Remove walls from the copied range.
    const State::WallList& removeWalls = step.removeWalls;
    for (State::WallList::const_iterator rmWall = removeWalls.begin();
         rmWall != removeWalls.end();
         ++rmWall)
    {
      WallRemovePos rmPos = std::remove(wallsCopyBegin, walls.end(), *rmWall);
      // Remove wall is valid?
      if (walls.end() != rmPos)
      {
        rmPos = walls.erase(rmPos);
        assert((walls.end() == rmPos) && "Walls must be unique.");
      }
      else
      {
        std::cerr << "DoStepH() : "
                  << "Player tried to delete non-existent wall." << std::endl;
        err |= PlyError::RemoveWallNotExist;
        break;
      }
    }
  }
  // If there were no errors, then remove the walls, add new walls, advance H,
  // and check for collisions.
  if (PlyError::Success == err)
  {
    if (wantRemoveWall)
    {
      const int wallsNow = numWalls - static_cast<int>(step.removeWalls.size());
      std::copy(walls.begin() + numWalls, walls.end(), walls.begin());
      walls.resize(wallsNow);
      std::make_heap(walls.begin(), walls.end(), State::Wall::Sort());
    }
    if (wantCreateWall)
    {
      walls.push_back(wall);
      std::push_heap(walls.begin(), walls.end(), State::Wall::Sort());
    }
    // reissb -- 20111030 -- We do not have to check collision with the new wall
    //   since H moves diagonally. This guarantees that he will not collide with
    //   any new walls.
    MovePlayer(*state, &state->motionH);
  }
  // Reset any recoverable mutations to data when there is an error.
  else
  {
    // Used temporary data at the end of wall; release it.
    walls.resize(numWalls);
  }
  return err;
}
/// <summary> Internal helper function to undo an H step. </summary>
inline void UndoStepH(const StepH& step, State* state)
{
  State::WallList& walls = state->walls;
  // Undo H motion.
  state->motionH.d *= -1;
  MovePlayer(*state, &state->motionH);
  state->motionH.d *= -1;
  // Remove added wall (order maintained by sorting).
  if (StepH::WallCreate_None != step.wallCreateFlag)
  {
    std::pop_heap(walls.begin(), walls.end(), State::Wall::Sort());
    walls.pop_back();
  }
  // Replace removed walls and maintain order.
  for (State::WallList::const_iterator rmWall = step.removeWalls.begin();
       rmWall != step.removeWalls.end();
       ++rmWall)
  {
    walls.push_back(*rmWall);
    std::push_heap(walls.begin(), walls.end(), State::Wall::Sort());
  }
}
}

PlyError DoPly(const StepH& stepH, State* state)
{
  assert(state);
  PlyError err = detail::DoStepH(stepH,  state);
  if (PlyError::Success == err)
  {
    ++state->simTime;
  }
  return err;
}

PlyError DoPly(const StepH& stepH, const StepP& stepP, State* state)
{
  assert(state);
  return PlyError(PlyError::Success);
}

}
}
