#include "evasion_core.h"
#include "process.h"

/// <summary> Write the Hunter's move to the client. </summary>
/// <remarks>
///   <para> The hunter 
/// </remarks>
void WriteHunterMove(hps::Process* evasionClient,
                     const hps::StepH& first, const hps::StepH& second)
{
  // The format for the hunter move comes from evasion_game_server.py:
  //   r = re.compile('Remove:\[(.*)\] Build:(.*) Remove:\[(.*)\] Build:(.*)')
}

/// <summary> Read the Hunter's move from the server. </summary>
bool ReadState(hps::Process* evasionClient, hps::State* state)
{
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
}

// TODO(reissb) -- 20111101 -- Create main method for evasion.
//   1) Connect to evasion client.
//   2) Begin processing moves and send to client.
int main(int /*argc*/, char** /*argv*/)
{
  return 0;
}
