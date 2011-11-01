#include "evasion_core.h"
#include "wall_clip_util.h"
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
  wall.simTimeCreate = state->simTime;
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
      state->simTimeLastWall = wall.simTimeCreate;
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
  // Remove added wall since it will block reversing the motion.
  // The wall order is maintained by sorting.
  if (StepH::WallCreate_None != step.wallCreateFlag)
  {
    assert((state->simTime - 1) == walls.front().simTimeCreate);
    std::pop_heap(walls.begin(), walls.end(), State::Wall::Sort());
    walls.pop_back();
    if (walls.empty())
    {
      state->simTimeLastWall = -state->wallCreatePeriod;
    }
    else
    {
      state->simTimeLastWall = walls.front().simTimeCreate;
    }
  }
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

bool ReadToken(const int idx, std::istream* stream, std::string* token)
{
  assert(stream && token);
  for (int tokenIdx = 0; tokenIdx < idx; ++tokenIdx)
  {
    if (stream->good())
    {
      std::string burn;
      *stream >> burn;
    }
    else
    {
      return false;
    }
  }
  if (stream->good())
  {
    token->clear();
    *stream >> *token;
    return true;
  }
  else
  {
    return false;
  }
}

struct TokenIterator
{
  TokenIterator(std::istream* stream_) : stream(stream_), idx(0) {}
  TokenIterator(std::istream* stream_, int idx_) : stream(stream_), idx(idx_) {}
  bool Next(const int tokenIdx, std::string* token)
  {
    const int offset = tokenIdx - idx;
    assert(offset >= 0);
    idx = tokenIdx + 1;
    return ReadToken(offset, stream, token);
  }
  std::istream* stream;
  int idx;
};

// Walls: [wall_index (x0, y0, x1, y1), wall_index (x0, y0, x1, y1), ...]
struct PyArrPunctuationToChar
{
  PyArrPunctuationToChar(char c_) : c(c_) {}
  inline char operator()(const char in) const
  {
    if ((',' == in) || ('(' == in) || (')' == in) || ('[' == in) || (']' == in))
    {
      return c;
    }
    else
    {
      return in;
    }
  }
  char c;
};

template <typename Type>
void ExtractToken(const std::string& token, Type* out)
{
  assert(out);
  std::stringstream ssToken(token);
  ssToken >> *out;
}

bool ParseStateString(const std::string& stateStr, const int m, const int n,
                      State* state)
{
  assert(state);
  // Board output is specified in evasion_game_server.py:
  //   1| conn1.send("You are Hunter\n" + str(board))
  // -OR-
  //   1| conn1.send("You are Prey\n" + str(board))
  //  -WHERE str(hunter) is-
  //   2| return "Hunter:" + str(self.x) + " " + str(self.y) + " " +
  //                         str(self.delta_x) + " " + str(self.delta_y) + "\n"
  //  -WHERE str(prey) is-
  //   3| return "Prey:  " + str(self.x) + " " + str(self.y) + "\n"
  //  -WHERE str(board) is-
  //   4| out += "Walls: ["
  //      wall_strs = []
  //      for i, wall in enumerate(self.walls[:-4]):
  //      if wall:
  //      if wall[0] == 1:
  //      wall_strs.append("%d (%d, %d, %d, %d)" % (i, wall[3], wall[1], wall[3], wall[2]))
  //      else:
  //      wall_strs.append("%d (%d, %d, %d, %d)" % (i, wall[1], wall[3], wall[2], wall[3]))
  //      out += ", ".join(wall_strs) + "]\n"
  //      return out
  //
  // The following were recovered from the console.
  // ***
  // You are Hunter
  // Hunter:0 0 1 1
  // Prey:  330 200
  // Walls: []
  // 
  // ***
  // You are Prey
  // Hunter:0 0 1 1
  // Prey:  330 200
  // Walls: []
  // 
  // ***
  Initialize(m, n, state);
  enum { BoardStateLines = 4, };
  enum { Hunter, Prey };
  std::stringstream ssLines;
  ssLines.str(stateStr);
  int player;
  // Line 1.
  {
    enum { Token_Player = 2, };
    std::string strLine;
    std::getline(ssLines, strLine);
    std::stringstream ssLine(strLine);
    std::string strPlayer;
    if (!ReadToken(Token_Player, &ssLine, &strPlayer))
    {
      return false;
    }
    if ("Hunter" == strPlayer)
    {
      player = Hunter;
    }
    else
    {
      assert("Prey" == strPlayer);
      player = Prey;
    }
  }
  // Line 2.
  State::HunterMotionInfo& motionH = state->motionH;
  {
    enum { Token_Px = 1, Token_Py, Token_Dx, Token_Dy, };
    std::string strLine;
    std::getline(ssLines, strLine);
    // Replace ':' with ' ' since output has no space delim after colon.
    const size_t colonPos = strLine.find_first_of(':', 0);
    if (std::string::npos == colonPos)
    {
      return false;
    }
    strLine[colonPos] = ' ';
    std::stringstream ssLine(strLine);
    TokenIterator tokenIter(&ssLine);
    // Px
    {
      std::string token;
      if (!tokenIter.Next(Token_Px, &token))
      {
        return false;
      }
      ExtractToken(token, &motionH.pos.x);
    }
    // Py
    {
      std::string token;
      if (!tokenIter.Next(Token_Py, &token))
      {
        return false;
      }
      std::stringstream ssTok(token);
      ssTok >> motionH.pos.y;
    }
    // Dx
    {
      std::string token;
      if (!tokenIter.Next(Token_Dx, &token))
      {
        return false;
      }
      std::stringstream ssTok(token);
      ssTok >> motionH.dir.x;
    }
    // Dy
    {
      std::string token;
      if (!tokenIter.Next(Token_Dy, &token))
      {
        return false;
      }
      std::stringstream ssTok(token);
      ssTok >> motionH.dir.y;
    }
  }
  // Line 3.
  State::Position& posP = state->posStackP.front();
  {
    enum { Token_Px = 1, Token_Py, };
    std::string strLine;
    std::getline(ssLines, strLine);
    std::stringstream ssLine(strLine);
    TokenIterator tokenIter(&ssLine);
    // Px
    {
      std::string token;
      if (!tokenIter.Next(Token_Px, &token))
      {
        return false;
      }
      std::stringstream ssTok(token);
      ssTok >> posP.x;
    }
    // Py
    {
      std::string token;
      if (!tokenIter.Next(Token_Py, &token))
      {
        return false;
      }
      std::stringstream ssTok(token);
      ssTok >> posP.y;
    }
  }
  // Line 4.
  // Walls: [wall_index (x0, y0, x1, y1), wall_index (x0, y0, x1, y1), ...]
  {
    enum { Token_Idx = 0, Token_x0, Token_y0, Token_x1, Token_y1, };
    std::string strLine;
    std::getline(ssLines, strLine);
    std::string strNoPunctuation(strLine);
    std::transform(strLine.begin(), strLine.end(),
                   strNoPunctuation.begin(), PyArrPunctuationToChar(' '));
    std::stringstream ssLine(strLine);
    // Ignore "Walls:"
    {
      std::string burn;
      if (!ReadToken(0, &ssLine, &burn))
      {
        return false;
      }
    }
    TokenIterator tokenIter(&ssLine);
    // Read walls.
    int wallCount = 0;
    while (ssLine.good())
    {
      // Read wall_index.
      int wallIdx;
      {
        std::string token;
        if (!tokenIter.Next(Token_Idx * wallCount, &token))
        {
          return false;
        }
        std::stringstream ssTok(token);
        ssTok >> posP.x;
      }
      ++wallCount;
      // Record this wall mapping.
      const int wallId = -wallCount;
      state->mapSimTimeToIdx[wallId] = wallIdx;
    }
  }
  return true;
}

}
}
