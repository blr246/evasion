#include "evasion_core.h"

namespace hps
{
namespace evasion
{

State::MotionInfo::MotionInfo() : s(), d() {}

State::MotionInfo::MotionInfo(const Position& s_, const Direction& d_)
: s(s_),
  d(d_)
{}

State::State()
  : simTime(0),
    motionP(Position(PInitialPosition::X, PInitialPosition::Y), Direction(0, 0)),
    motionH(Position(HInitialPosition::X, HInitialPosition::Y), Direction(1, 1)),
    walls(),
    board(Vector2<int>(0, 0), Vector2<int>(BoardSizeX, BoardSizeY)),
    wallCreatePeriod(0),
    maxWalls(0)
{}

}
}
