#ifndef _HPS_SYS_PROCESS_GTEST_H_
#define _HPS_SYS_PROCESS_GTEST_H_
#include "process.h"
#include "gtest/gtest.h"
#include <vector>

namespace _hps_sys_process_gtest_h_
{
using namespace hps;

TEST(process, SafeCoverage)
{
  Process process;
  EXPECT_EQ(0, process.Join());
  process.Kill();
  std::string str;
  process.ReadStdout(&str);
  EXPECT_TRUE(str.empty());
}

TEST(process, StartAgain)
{
  Process process;
#if WIN32
  std::vector<std::string> args;
  {
    args.push_back("cmd");
    args.push_back("/C");
    args.push_back("date /T");
  }
  EXPECT_TRUE(process.Start(args));
  EXPECT_TRUE(process.Start(args));
#else
  EXPECT_TRUE(process.Start("date"));
  EXPECT_TRUE(process.Start("date"));
#endif

}

TEST(process, write_read)
{
  Process process;
  std::vector<std::string> args;
  {
    args.push_back("python");
    args.push_back("-c");
    args.push_back("print raw_input()");
  }
  process.Start(args);
  const std::string writeToProcess("hello");
  process.WriteStdin(writeToProcess + std::string("\r\n"));
  std::string readFromProcess;
  process.ReadStdout(&readFromProcess);
  process.Join();
  EXPECT_TRUE(writeToProcess == readFromProcess.substr(0, writeToProcess.size()));
}

TEST(process, Python)
{
  Process process;
  std::vector<std::string> args(3);
  {
    args[0] = "python";
    args[1] = "-c";
    args[2] = "print 2 * 2";
  }
  ASSERT_TRUE(process.Start(args));
  std::string out;
  std::cout << "Reading from child STDOUT." << std::endl;
  process.ReadStdout(&out);
  std::cout << "Waiting for child to terminate." << std::endl;
  int status = process.Join();
  std::cout << "Child terminated with status " << status << "." << std::endl;
  std::cout << "Child output: " << out << std::endl;
  ASSERT_FALSE(out.empty());
  EXPECT_EQ('4', out[0]);
}

}

#endif //_HPS_SYS_PROCESS_GTEST_H_
