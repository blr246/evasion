#include "evasion_core.h"
#include "prey.h"
#include "hunter.h"
#include "process.h"
#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>

using namespace hps;
/// <summary> Write the Hunter's move to the client. </summary>
void WriteHunterMove(hps::Process* evasionClient,
                     BasicHunter h,
                     State *state)
{
  // The format for the hunter move comes from evasion_game_server.py:
  //   r = re.compile('Remove:\[(.*)\] Build:(.*) Remove:\[(.*)\] Build:(.*)')
  State::Wall built;
  std::vector<State::Wall> removed;
  StepH first = h.Play(state,&built,&removed);
  DoPly(first, state);
  evasionClient->WriteStdin(first.Serialize(*state));
  StepH second = h.Play(state,&built,&removed);
  DoPly(second, state);
  evasionClient->WriteStdin(second.Serialize(*state));
  evasionClient->WriteStdin("\n");
}

/// <sumamry> Write the Prey's move to the client. </summary>
void WritePreyMove(hps::Process* evasionClient, hps::StepP& stepP)
{
  evasionClient->WriteStdin(stepP.Serialize());
  evasionClient->WriteStdin("\n");
}

/// <summary> Read the Hunter's move from the server. </summary>
bool ReadState(hps::Process* evasionClient, const int m, const int n, hps::State* state)
{
  // Read the lines from the server and parse them.
  std::string strLines;
  evasionClient->ReadStdout(&strLines);
  if (strLines.empty())
  {
    return false;
  }
  return hps::ParseStateString(strLines, m, n, state);
}

struct Args
{
  enum
  {
    Argv_Hostname = 0,
    Argv_Port,
    Argv_M,
    Argv_N,
    Argv_Count,
  };
  Args() : asList(Argv_Count) {}
  std::vector<std::string> asList;
  std::string hostname;
  int port;
  int m;
  int n;
};

// TODO(reissb) -- 20111101 -- Create main method for evasion.
//   1) Connect to evasion client.
//   2) Begin processing moves and send to client.
int main(int argc, char** argv)
{
  if (argc != Args::Argv_Count + 1)
  {
    std::cerr << "Usage: evasion HOSTNAME PORT M N" << std::endl;
    return 1;
  }

  // Extract args.
  Args args;
  {
    // asList
    for (int argIdx = 0; argIdx < Args::Argv_Count; ++argIdx)
    {
      args.asList[argIdx] = argv[argIdx + 1];
    }
    // hostname
    {
      args.hostname = args.asList[Args::Argv_Hostname];
    }
    // port
    {
      std::stringstream ssArg(args.asList[Args::Argv_Port]);
      ssArg >> args.port;
    }
    // m
    {
      std::stringstream ssArg(args.asList[Args::Argv_M]);
      ssArg >> args.m;
    }
    // n
    {
      std::stringstream ssArg(args.asList[Args::Argv_N]);
      ssArg >> args.n;
    }
  }

  // Open process to client.
  enum { EvasionClientExtraArgs = 2, };
  hps::Process evasionClient;
  std::vector<std::string> evasionClientArgs(Args::Argv_Count + EvasionClientExtraArgs);
  evasionClientArgs[0] = "python";
  evasionClientArgs[1] = "evasion_game_client.py";
  std::copy(args.asList.begin(), args.asList.end(),
            evasionClientArgs.begin() + EvasionClientExtraArgs);
  if (!evasionClient.Start(evasionClientArgs))
  {
    std::cerr << "Unable to start evasion game client using command:"
              << std::endl << " ";
    for (int argIdx = 0; argIdx < evasionClientArgs.size(); ++argIdx)
    {
      std::cerr << " " << evasionClientArgs[argIdx];
    }
    std::cerr << std::endl;
    return 2;
  }
  else
  {
    std::cout << "Started evasion game client using command:"
              << std::endl << " ";
    for (int argIdx = 0; argIdx < evasionClientArgs.size(); ++argIdx)
    {
      std::cout << " " << evasionClientArgs[argIdx];
    }
    std::cout << std::endl;
  }

  // Read initial state.
  hps::State state;
  while (ReadState(&evasionClient, args.m, args.n, &state))
  {
    // Play the game!
    bool isPrey;
    std::string s;
    if(isPrey){
      ExtremePrey p;
      StepP step = p.NextStep(state);
      WritePreyMove(&evasionClient, step);
    }else{
      BasicHunter h;
      WriteHunterMove(&evasionClient, h, &state);
    }
  
  }

  return 0;
}
