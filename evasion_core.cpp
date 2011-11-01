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
    if (PlyError::PreyCaptured & err)
    {
      detail::StreamFlagsPrependDelim<'|'>(flags, &strm);
      ssErr << "PreyCaptured";
    }
    strm << ssErr.str();
  }
  return strm;
}

State::State()
: simTime(0),
  simTimeLastWall(-1),
  posStackP(),
  motionH(),
  walls(),
  board(Vector2<int>(0, 0), Vector2<int>(BoardSizeX - 1, BoardSizeY - 1)),
  wallCreatePeriod(0),
  maxWalls(0),
  preyState(PreyState_Evading)
{
  posStackP.reserve(PosStackPrealloc);
  posStackP.push_back(Position(InitialPosition::Prey::X,
                               InitialPosition::Prey::Y));
}

namespace detail
{
template <int WallDirection>
struct WallClipHelper;

/// <summary> Coordinate helper for horizontal wall clipping. </summary>
template <>
struct WallClipHelper<State::Wall::Type_Horizontal>
{
    enum { OtherDirection = State::Wall::Type_Vertical, };
  inline static const int& GetFixed(const State::Position& pt) { return pt.y; }
  inline static int& GetFixed(State::Position& pt) { return pt.y; }
  inline static const int& GetClip(const State::Position& pt) { return pt.x; }
  inline static int& GetClip(State::Position& pt) { return pt.x; }
  inline static const State::Wall::Coordinates InitWallCoords(const int fixed)
    {
    return State::Wall::Coordinates(State::Position(0, fixed),
                                    State::Position(BoardSizeX, fixed));
    }
};

/// <summary> Coordinate helper for vertical wall clipping. </summary>
template <>
struct WallClipHelper<State::Wall::Type_Vertical>
{
    enum { OtherDirection = State::Wall::Type_Horizontal, };
  inline static const int& GetFixed(const State::Position& pt) { return pt.x; }
  inline static int& GetFixed(State::Position& pt) { return pt.x; }
  inline static const int& GetClip(const State::Position& pt) { return pt.y; }
  inline static int& GetClip(State::Position& pt) { return pt.y; }
  inline static const State::Wall::Coordinates InitWallCoords(const int fixed)
    {
    return State::Wall::Coordinates(State::Position(fixed, 0),
                                    State::Position(fixed, BoardSizeY));
    }
};

/// <summary> Create a wall clipped by a list of existing walls. </summary>
template <int WallDirection>
State::Wall CreateClipped(const State::Position& pt,
                          const State::WallList::const_iterator& first,
                          const State::WallList::const_iterator& last)
{
  typedef State::WallList WallList;
  typedef State::Wall Wall;
  typedef State::Wall::Coordinates WallCoordinates;
  typedef WallClipHelper<WallDirection> CoordHelp;
  const int fixed = CoordHelp::GetFixed(pt);
  Wall wall;
  {
    wall.type = static_cast<State::Wall::Type>(WallDirection);
    wall.coords = CoordHelp::InitWallCoords(fixed);
  }
  WallCoordinates& wallCoords = wall.coords;
  // Clip wall using all perpendicular walls.
  for (WallList::const_iterator clipW = first; clipW != last; ++clipW)
  {
    const WallCoordinates& clipCoords = clipW->coords;
    // Does wall cross the that we're clipping?
    if ((static_cast<int>(CoordHelp::OtherDirection) == clipW->type) &&
        ((CoordHelp::GetFixed(clipCoords.p0) <= fixed) &&
         (CoordHelp::GetFixed(clipCoords.p1) >= fixed)))
    {
        // Clip wall toward H.
        assert(CoordHelp::GetClip(clipCoords.p0) == CoordHelp::GetClip(clipCoords.p1));
        const int clip = CoordHelp::GetClip(clipCoords.p0);
        if (clip <= CoordHelp::GetClip(pt))
        {
          int& clipped = CoordHelp::GetClip(wallCoords.p0);
          clipped = std::max(clip, clipped);
        }
        else
        {
          int& clipped = CoordHelp::GetClip(wallCoords.p1);
          clipped = std::min(clip, clipped);
        }
    }
  }
  return wall;
}

/// <summary> See if the point collides with the wall. </sumamry>
template <int WallDirection>
inline static bool CheckCollision(const State::Wall::Coordinates& wc,
                                  const State::Position& pt)
{
  typedef WallClipHelper<WallDirection> CoordHelp;
    assert(CoordHelp::GetFixed(wc.p0) == CoordHelp::GetFixed(wc.p1));
    return (CoordHelp::GetFixed(pt) == CoordHelp::GetFixed(wc.p0)) &&
           (CoordHelp::GetClip(wc.p0) <= CoordHelp::GetClip(pt)) &&
           (CoordHelp::GetClip(wc.p1) >= CoordHelp::GetClip(pt));
}

/// <summary> Advance the player's motion. </summary>
/// <remarks>
///   <para> Apply the motion info direction to the position. In the case of a
///     player that can move freely, the direction field must be updated for
///     the current time step prior to applying this function.
///   </para>
///   <para> Returns true only when the new position is reachable. </para>
/// </remarks>
bool MovePlayer(const State::Board& board,
                const State::WallList::const_iterator& wallFirst,
                const State::WallList::const_iterator& wallLast,
                const State::Position& pos, const State::Direction& dir,
                State::Position* newPos, State::Direction* newDir)
{
  assert(newPos && newDir);
  // Nothing to do when player is stationary.
  static const State::Direction moveZero(0, 0);
  if (moveZero == dir)
  {
    *newDir = dir;
    *newPos = pos;
    return true;
  }
  const State::Position posNew = pos + dir;
  bool hitWallH = false;
  bool hitWallV = false;
  // Check collision with board.
  if ((posNew.y < board.mins.y) || (board.maxs.y < posNew.y))
  {
    hitWallH = true;
  }
  if ((posNew.x < board.mins.x) || (board.maxs.x < posNew.x))
  {
    hitWallV = true;
  }
  // Check collision with all walls. Check directions independently.
  const State::Position posNewV(pos.x, posNew.y); // Hits horizontal wall
  const State::Position posNewH(posNew.x, pos.y); // Hits vertical wall
  bool moveInvalid = false;
  for (State::WallList::const_iterator wall = wallFirst; wall != wallLast; ++wall)
  {
    const State::Wall::Coordinates coords = wall->coords;
    if (State::Wall::Type_Horizontal == wall->type)
    {
      hitWallH |= CheckCollision<State::Wall::Type_Horizontal>(coords, posNewV);
      hitWallH |= (coords.p0 == posNewV) || (coords.p1 == posNewV);
      moveInvalid |= !hitWallH &&
                     CheckCollision<State::Wall::Type_Horizontal>(wall->coords, posNew);
    }
    else
    {
      assert(State::Wall::Type_Vertical == wall->type);
      hitWallV |= CheckCollision<State::Wall::Type_Vertical>(coords, posNewH);
      hitWallV |= (coords.p0 == posNewH) || (coords.p1 == posNewH);
      moveInvalid |= !hitWallV &&
                     CheckCollision<State::Wall::Type_Vertical>(wall->coords, posNew);
    }
  }
  if (!moveInvalid)
  {
    // Perform reflection. Bounce off of wall type.
    *newDir = dir;
    if (hitWallH)
    {
      newDir->y *= -1;
    }
    if (hitWallV)
    {
      newDir->x *= -1;
    }
    // Move player. Do not advance reflected coordinates.
    *newPos = pos + State::Direction(!hitWallV * dir.x, !hitWallH * dir.y);
    return true;
  }
  else
  {
    return false;
  }
}
/// <summary> Internal helper function to apply an H step. </summary>
PlyError DoStepH(const StepH& step, State* state)
{
  assert(state);
  PlyError err(PlyError::Success);
  State::WallList& walls = state->walls;
  State::WallList& wallsTemp = state->wallsTemp;
  wallsTemp = walls;
  State::Wall wall;
  // Record if Hunter is stuck.
  const bool stuckBefore = (state->motionH.simTimeStuck !=
                            State::HunterMotionInfo::Flag_NotStuck);
  bool unstuck = false;
  // See if wall removals are valid. Use wallsTemp as temporary storage.
  const bool wantRemoveWall = !step.removeWalls.empty();
  if (wantRemoveWall)
  {
    typedef State::WallList::iterator WallRemovePos;
    // Remove walls from the copied range.
    const State::Position& posH = state->motionH.pos;
    const State::WallList& removeWalls = step.removeWalls;
    for (State::WallList::const_iterator rmWall = removeWalls.begin();
         rmWall != removeWalls.end();
         ++rmWall)
    {
      WallRemovePos rmPos = std::remove(wallsTemp.begin(), wallsTemp.end(),
                                        *rmWall);
      // See if we are unstuck by a wall removal.
      if (stuckBefore && !unstuck)
      {
        if (State::Wall::Type_Horizontal == rmWall->type)
        {
          unstuck = CheckCollision<State::Wall::Type_Horizontal>(rmWall->coords, posH);
        }
        else
        {
          assert(State::Wall::Type_Vertical == rmWall->type);
          unstuck = CheckCollision<State::Wall::Type_Vertical>(rmWall->coords, posH);
        }
      }
      // Remove wall is valid?
      if (wallsTemp.end() != rmPos)
      {
        rmPos = wallsTemp.erase(rmPos);
        assert((wallsTemp.end() == rmPos) && "Walls must be unique.");
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
  // Get Hunter's new position.
  State::HunterMotionInfo motionHNew;
  if (!stuckBefore || unstuck)
  {
     const bool stuckNow = !MovePlayer(state->board,
                                       wallsTemp.begin(), wallsTemp.end(),
                                       state->motionH.pos, state->motionH.dir,
                                       &motionHNew.pos, &motionHNew.dir);
     if (stuckNow)
     {
       motionHNew.simTimeStuck = state->simTime + 1;
     }
  }
  else
  {
    motionHNew = state->motionH;
  }
  // See if wall creation requested and allowed. Use walls modified by removals.
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
    // Check wall creations.
    {
      // See if wall will intersect P by first creating the wall.
      const State::Position& posH = state->motionH.pos;
      const State::Position& posP = state->posStackP.back();
      if (StepH::WallCreate_Horizontal == step.wallCreateFlag)
      {
        assert(StepH::WallCreate_Horizontal == step.wallCreateFlag);
        wall = CreateClipped<State::Wall::Type_Horizontal>(posH,
                                                           wallsTemp.begin(), wallsTemp.end());
        wall.simTimeCreate = state->simTime + 1;
        assert(State::Wall::Type_Horizontal == wall.type);
        // Check interference.
        if (CheckCollision<State::Wall::Type_Horizontal>(wall.coords, posP))
        {
          std::cerr << "DoStepH() : Creation of horizontal wall cannot proceed "
                    << "since P is in the way." << std::endl;
          err |= PlyError::WallCreateInterference;
        }
        if (CheckCollision<State::Wall::Type_Horizontal>(wall.coords, motionHNew.pos))
        {
          std::cerr << "DoStepH() : Creation of horizontal wall cannot proceed "
                    << "since H moves in the way." << std::endl;
          err |= PlyError::WallCreateInterference;
        }
      }
      else
      {
        assert(StepH::WallCreate_Vertical == step.wallCreateFlag);
        wall = CreateClipped<State::Wall::Type_Vertical>(posH,
                                                         wallsTemp.begin(), wallsTemp.end());
        assert(State::Wall::Type_Vertical == wall.type);
        // Check interference.
        if (CheckCollision<State::Wall::Type_Vertical>(wall.coords, posP))
        {
          std::cerr << "DoStepH() : Creation of vertical wall cannot proceed "
                    << "since P is in the way." << std::endl;
          err |= PlyError::WallCreateInterference;
        }
        if (CheckCollision<State::Wall::Type_Vertical>(wall.coords, motionHNew.pos))
        {
          std::cerr << "DoStepH() : Creation of vertical wall cannot proceed "
                    << "since H moves in the way." << std::endl;
          err |= PlyError::WallCreateInterference;
        }
      }
    }
  }
  // If there were no errors, then remove the walls, add new walls, advance H,
  // and check for collisions.
  if (PlyError::Success == err)
  {
    if (wantRemoveWall)
    {
      walls = wallsTemp;
      std::make_heap(walls.begin(), walls.end(), State::Wall::Sort());
    }
    if (wantCreateWall)
    {
      walls.push_back(wall);
      std::push_heap(walls.begin(), walls.end(), State::Wall::Sort());
      state->simTimeLastWall = state->simTime;
    }
    state->motionH = motionHNew;
  }
  wallsTemp.clear();
  return err;
}
/// <summary> Internal helper function to undo an H step. </summary>
inline void UndoStepH(const StepH& step, State* state)
{
  State::WallList& walls = state->walls;
  // Undo H motion if he isnt stuck.
  if (state->motionH.simTimeStuck == state->simTime)
  {
    state->motionH.simTimeStuck = State::HunterMotionInfo::Flag_NotStuck;
  }
  typedef State::HunterMotionInfo HunterMotionInfo;
  bool stuck = HunterMotionInfo::Flag_NotStuck != state->motionH.simTimeStuck;
  if (!stuck)
  {
    state->motionH.dir *= -1;
    MovePlayer(state->board, state->walls.begin(), state->walls.end(),
               state->motionH.pos, state->motionH.dir,
               &state->motionH.pos, &state->motionH.dir);
    state->motionH.dir *= -1;
  }
  // Remove added wall (order maintained by sorting).
  if (StepH::WallCreate_None != step.wallCreateFlag)
  {
    std::pop_heap(walls.begin(), walls.end(), State::Wall::Sort());
    walls.pop_back();
    assert(state->simTime == walls.front().simTimeCreate);
    if (walls.empty())
    {
      state->simTimeLastWall = -state->wallCreatePeriod;
    }
    else
    {
      state->simTimeLastWall = walls.front().simTimeCreate;
    }
  }
  // Replace removed walls and maintain order.
  const State::Position& posH = state->motionH.pos;
  for (State::WallList::const_iterator rmWall = step.removeWalls.begin();
       rmWall != step.removeWalls.end();
       ++rmWall)
  {
    // Check if H was stuck.
    if (!stuck)
    {
      if (State::Wall::Type_Horizontal == rmWall->type)
      {
        stuck = CheckCollision<State::Wall::Type_Horizontal>(rmWall->coords, posH);
      }
      else
      {
        assert(State::Wall::Type_Vertical == rmWall->type);
        stuck = CheckCollision<State::Wall::Type_Vertical>(rmWall->coords, posH);
      }
    }
    walls.push_back(*rmWall);
    std::push_heap(walls.begin(), walls.end(), State::Wall::Sort());
  }
  if (stuck)
  {
    state->motionH.simTimeStuck = state->simTime - 1;
  }
}

/// <summary> Try to capture the prey. </summary>
void TryCapturePrey(State* state)
{
  assert(State::PreyState_Evading == state->preyState);
  const State::Position vecHtoP = state->motionH.pos - state->posStackP.back();
  const int distHtoPSq = Vector2LengthSq(vecHtoP);
  const float distHtoP = sqrt(static_cast<float>(distHtoPSq));
  if (static_cast<int>(distHtoP) < HCatchesPDist)
  {
    state->preyState = State::PreyState_Captured;
  }
}
}

PlyError DoPly(const StepH& stepH, State* state)
{
  assert(state);
  // See if prey captured.
  PlyError err;
  if (PreyCaptured(*state))
  {
    std::cerr << "DoPly() : cannot advance Hunter-only ply since the prey is "
              << "captured already." << std::endl;
    err |= PlyError::PreyCaptured;
  }
  else
  {
    err |= detail::DoStepH(stepH, state);
    if (PlyError::Success == err)
    {
      ++state->simTime;
      // See if the prey is capture.
      detail::TryCapturePrey(state);
    }
  }
  return err;
}

void UndoPly(const StepH& stepH, State* state)
{
  assert(state);
  // reissb -- 20111031 -- Since the game stops once the prey is captured, we
  //   can just enforce that it is evading here.
  state->preyState = State::PreyState_Evading;
  detail::UndoStepH(stepH,  state);
  --state->simTime;
  assert(state->simTime >= 0);
}

PlyError DoPly(const StepH& stepH, const StepP& stepP, State* state)
{
  assert(state);
  // See if prey captured.
  PlyError err;
  if (PreyCaptured(*state))
  {
    std::cerr << "DoPly() : cannot advance mixed Hunter-Prey ply since the "
              << "prey is captured already." << std::endl;
    err |= PlyError::PreyCaptured;
  }
  else
  {
    // Move H first since he needs to know where P is when creating walls.
    err |= detail::DoStepH(stepH,  state);
    if (PlyError::Success == err)
    {
      ++state->simTime;
      // Move P second. He may bounce off of a new wall, so we have to check
      // the return code of MovePlayer().
      State::Position newPosP;
      State::Direction newDirP;
      const bool validMove = detail::MovePlayer(state->board,
                                                state->walls.begin(),
                                                state->walls.end(),
                                                state->posStackP.back(),
                                                stepP.moveDir,
                                                &newPosP, &newDirP);
      if (validMove)
      {
        state->posStackP.push_back(newPosP);
      }
      else
      {
        state->posStackP.push_back(state->posStackP.back());
      }
      detail::TryCapturePrey(state);
    }
  }
  return err;
}

void UndoPly(const StepH& stepH, const StepP& /*stepP*/, State* state)
{
  assert(state);
  // reissb -- 20111031 -- Since the game stops once the prey is captured, we
  //   can just enforce that it is evading here.
  state->preyState = State::PreyState_Evading;
  // Undo prey position.
  state->posStackP.pop_back();
  // Undo hunter's move.
  detail::UndoStepH(stepH,  state);
  --state->simTime;
  assert(state->simTime >= 0);
}

}
}
