#pragma once

#include <CMakeInstance.h>
#include <CommandInterpreter.h>
#include <Communicator.h>
#include <Socket.h>

class Debugger
{
public:
  Debugger(Args args)
    : m_command_interpreter()
    , m_args(args)
    , m_cmake_instance_sp(new CMakeInstance())
  {
  }
  void RunMainLoop();
  void RunCommandInterpreter();
  void LaunchCMakeInstance();

  CommandInterpreter& GetCommandInterpreter() { return m_command_interpreter; }
  CMakeInstance& GetCMakeInstance() { return *m_cmake_instance_sp; }

private:
  CommandInterpreter m_command_interpreter;
  CMakeInstanceSP m_cmake_instance_sp;
  Communicator m_communicator;
  Args m_args;
};
