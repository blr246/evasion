#ifndef _HPS_SYS_CHILD_PROCESS_H_
#define _HPS_SYS_CHILD_PROCESS_H_
#if WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#else
#include <sys/wait.h>
#include <assert.h>
#include <cstdio>
#include <cstdlib>
#endif
#ifdef __APPLE__
#include <signal.h>
#endif
#include <cstring>
#include <string>
#include <algorithm>
#include <iostream>

namespace hps
{
namespace sys
{


// Forward declarations.
class Process;
namespace detail
{
inline void CleanProcess(Process* process);
}

/// <summary> A child process. </summary>
class Process
{
  friend void detail::CleanProcess(Process* process);
public:
  enum { Fd_Read = 0, Fd_Write, Fd_Count, };

  Process();
  ~Process();

  bool Start(const std::vector<std::string>& args);
  inline bool Start(const std::string& cmd)
  {
    std::vector<std::string> args(1, cmd);
    return Start(args);
  }
  int Join();
  void Kill();
  int WriteStdin(const std::string& str);
  int ReadStdout(std::string* str);

protected:

private:
  Process(const Process&);
  Process& operator=(const Process&);

#if WIN32
  HANDLE m_process;
  HANDLE m_stdin[Fd_Count];
  HANDLE m_stdout[Fd_Count];
  HANDLE m_stderr[Fd_Count];
#else
  pid_t m_process;
  int m_stdin[Fd_Count];
  int m_stdout[Fd_Count];
  int m_stderr[Fd_Count];
#endif
};

namespace detail
{
template <typename HandleType>
inline void CloseHandles(HandleType handles[])
{
  for (int handleIdx = 0; handleIdx < Process::Fd_Count; ++handleIdx)
  {
#if WIN32
    CloseHandle(handles[handleIdx]);
#else
    close(handles[handleIdx]);
#endif
    handles[handleIdx] = NULL;
  }
}

inline void CleanProcess(Process* process)
{
  CloseHandles(process->m_stdin);
  CloseHandles(process->m_stdout);
  CloseHandles(process->m_stderr);
}
}

#if WIN32
#pragma warning(push)
#pragma warning(disable: 4351)
Process::Process()
: m_process(NULL),
  m_stdin(),
  m_stdout(),
  m_stderr()
{
  memset(m_stdin, 0, sizeof(m_stdin));
  memset(m_stdout, 0, sizeof(m_stdout));
  memset(m_stderr, 0, sizeof(m_stderr));
}
#pragma warning(pop)

#pragma warning(push)
#pragma warning(disable: 4996)
bool Process::Start(const std::vector<std::string>& args)
{ 
  // Make sure this is not running.
  Kill();
  detail::CleanProcess(this);

  // Set up the security attributes struct.
  SECURITY_ATTRIBUTES sa;
  {
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;
  }
  // Create a pipe for the child process's STDOUT. 
  if (!CreatePipe(&m_stdout[Fd_Read], &m_stdout[Fd_Write], &sa, 0)) 
  {
    return false;
  }
  // Ensure the read handle to the pipe for STDOUT is not inherited.
  if (!SetHandleInformation(m_stdout[Fd_Read], HANDLE_FLAG_INHERIT, 0))
  {
    detail::CleanProcess(this);
    return false;
  }
  // Create a pipe for the child process's STDIN. 
  if (!CreatePipe(&m_stdin[Fd_Read], &m_stdin[Fd_Write], &sa, 0)) 
  {
    detail::CleanProcess(this);
    return false;
  }
  // Ensure the write handle to the pipe for STDIN is not inherited. 
  if (!SetHandleInformation(m_stdin[Fd_Write], HANDLE_FLAG_INHERIT, 0))
  {
    detail::CleanProcess(this);
    return false;
  }
  // Create a pipe for the child process's STDERR.
  if (!CreatePipe(&m_stderr[Fd_Read], &m_stderr[Fd_Write], &sa, 0)) 
  {
    return false;
  }
  // Ensure the read handle to the pipe for STDERR is not inherited.
  if (!SetHandleInformation(m_stderr[Fd_Read], HANDLE_FLAG_INHERIT, 0))
  {
    detail::CleanProcess(this);
    return false;
  }
  // Set up members of the STARTUPINFO structure. 
  // This structure specifies the STDIN and STDOUT handles for redirection.
  STARTUPINFO startupInfo;
  memset(&startupInfo, 0, sizeof(startupInfo));
  {
    startupInfo.cb = sizeof(startupInfo);
    startupInfo.dwFlags = STARTF_USESTDHANDLES;
    startupInfo.hStdOutput = m_stdout[Fd_Write];
    startupInfo.hStdInput  = m_stdin[Fd_Read];
    startupInfo.hStdError  = m_stderr[Fd_Write];
  }
  // Join command line.
  enum { MaxCmdLine = 2048, };
  char cstrCmdLine[MaxCmdLine] = {0};
  {
    char* cstrCmdLinePtr = cstrCmdLine;
    char* const cstrCmdLineEndPtr = cstrCmdLine + MaxCmdLine;
    for (int argIdx = 0; argIdx < static_cast<int>(args.size()); ++argIdx)
    {
      const std::string& arg = args[argIdx];
      // Whitespace in this arg? Then quote it!
      const bool quoteIt = (std::string::npos != arg.find(' '));
      if (quoteIt)
      {
        assert((cstrCmdLinePtr + arg.size() + 3) < cstrCmdLineEndPtr);
        *cstrCmdLinePtr = '"';
        ++cstrCmdLinePtr;
      }
      else
      {
        assert((cstrCmdLinePtr + arg.size() + 1) < cstrCmdLineEndPtr);
      }
      // Insert the arg.
      std::copy(arg.begin(), arg.end(), cstrCmdLinePtr);
      cstrCmdLinePtr += arg.size();
      // Finish quoting.
      if (quoteIt)
      {
        *cstrCmdLinePtr = '"';
        ++cstrCmdLinePtr;
      }
      // Append space for next arg.
      *cstrCmdLinePtr = ' ';
      ++cstrCmdLinePtr;
    }
    // Null terminate joined command line.
    *(cstrCmdLinePtr - 1) = '\0';
  }
  // Create the child process. 
  PROCESS_INFORMATION processInfo; 
  if (!CreateProcess(NULL, cstrCmdLine,
                     NULL, NULL, TRUE, 0, NULL, NULL,
                     &startupInfo, &processInfo))
  {
    detail::CleanProcess(this);
    return false;
  }
  m_process = processInfo.hProcess;
  CloseHandle(processInfo.hThread);
  return true;
}
#pragma warning(pop)

int Process::Join()
{
  DWORD exitCode = 0;
  if (NULL != m_process)
  {
    WaitForSingleObject(m_process, INFINITE);
    GetExitCodeProcess(m_process, &exitCode);
  }
  detail::CleanProcess(this);
  return static_cast<int>(exitCode);
}

void Process::Kill()
{
  TerminateProcess(m_process, 0);
  detail::CleanProcess(this);
}

int Process::WriteStdin(const std::string& str)
{
  DWORD numWritten;
  WriteFile(m_stdin[Fd_Write], str.c_str(), str.size(), &numWritten, NULL);
  return static_cast<int>(numWritten);
}

int Process::ReadStdout(std::string* str)
{
  assert(str);
  str->clear();
  char strBuff[4096] = {0};
  DWORD numRead;
  ReadFile(m_stdout[Fd_Read], strBuff, sizeof(strBuff), &numRead, NULL);
  str->assign(strBuff);
  return static_cast<int>(numRead);
}
#else

Process::Process()
: m_process(0),
  m_stdin(),
  m_stdout(),
  m_stderr()
{
  memset(m_stdin, 0, sizeof(m_stdin));
  memset(m_stdout, 0, sizeof(m_stdout));
  memset(m_stderr, 0, sizeof(m_stderr));
}

bool Process::Start(const std::vector<std::string>& args)
{
  char buf;
  if ((-1 == pipe(m_stdout)) || (-1 == pipe(m_stdin)) || (-1 == pipe(m_stderr)))
  {
    detail::CleanProcess(this);
    std::cerr << "Failed to open pipes." << std::endl;
    return false;
  }
  pid_t process = vfork();
  if (-1 == process)
  {
    detail::CleanProcess(this);
    std::cerr << "Failed to vfork()." << std::endl;
    return false;
  }
  // Am I the child?
  if (0 == process)
  {
    // Connect pipes.
    dup2(m_stdin[Fd_Read], STDIN_FILENO);
    dup2(m_stdout[Fd_Write], STDOUT_FILENO);
    dup2(m_stderr[Fd_Write], STDERR_FILENO);
    // Extract the args.
    enum { MaxArgs = 256, };
    enum { MaxCmdLine = 2048, };
    char* argsCstr[MaxArgs] = {0};
    char cmdLineCstr[MaxCmdLine] = {0};
    {
      char** argsCstrPtr = argsCstr;
      char** const argsCstrEndPtr = argsCstr + MaxArgs;
      char* argCstrPtr = cmdLineCstr;
      const char* argCstrEndPtr = cmdLineCstr + MaxCmdLine;
      for (int argIdx = 0; argIdx < static_cast<int>(args.size()); ++argIdx)
      {
        // Copy the next argument into the buffer.
        assert(argsCstrPtr < argsCstrEndPtr);
        assert(argCstrPtr < argCstrEndPtr);
        const std::string arg = args[argIdx];
        assert((argCstrPtr + arg.size()) < argCstrEndPtr);
        strcpy(argCstrPtr, arg.c_str());
        assert('\0' == *(argCstrPtr + arg.size()));
        // Set this arg and advance pointers.
        *argsCstrPtr = argCstrPtr;
        ++argsCstrPtr;
        argCstrPtr += arg.size() + 1;
      }
    }
    if (-1 == execvp(argsCstr[0], argsCstr))
    {
      exit(-1);
    }
  }
  else
  {
    m_process = process;
    return true;
  }
}

int Process::Join()
{
  siginfo_t info;
  memset(&info, 0, sizeof(info));
  if (m_process > 0)
  {
  waitid(P_PID, m_process, &info, WEXITED);
  detail::CleanProcess(this);
  }
  return info.si_status;
}

void Process::Kill()
{
  if (m_process > 0)
  {
  kill(m_process, SIGKILL);
  detail::CleanProcess(this);
  }
  m_process = 0;
}

int Process::WriteStdin(const std::string& str)
{
  int numWritten = 0;
  write(m_stdin[Fd_Write], str.c_str(), str.size());
  return numWritten;
}

int Process::ReadStdout(std::string* str)
{
  assert(str);
  str->clear();
  int numRead = 0;
  if (m_process > 0)
  {
  char strBuff[4096] = {0};
  read(m_stdout[Fd_Read], strBuff, sizeof(strBuff));
  str->assign(strBuff);
  }
  return numRead;
}
#endif

Process::~Process()
{
  Kill();
  detail::CleanProcess(this);
}

}
using namespace sys;
}

#endif //_HPS_SYS_CHILD_PROCESS_H_
