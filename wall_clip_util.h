#ifndef _HPS_EVASION_WALL_CLIP_UTIL_H_
#define _HPS_EVASION_WALL_CLIP_UTIL_H_
#include "evasion_core.h"

namespace hps
{
namespace evasion
{
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
                                    State::Position(BoardSizeX - 1, fixed));
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
                                    State::Position(fixed, BoardSizeY - 1));
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
          clipped = std::max(clip + 1, clipped);
        }
        else
        {
          int& clipped = CoordHelp::GetClip(wallCoords.p1);
          clipped = std::min(clip - 1, clipped);
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

}
}
}

#endif //_HPS_EVASION_WALL_CLIP_UTIL_H_
