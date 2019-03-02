#pragma once

#include <CMakeInstance.h>
#include <CommandInterpreter.h>
#include <Communicator.h>
#include <Socket.h>

class Debugger
{
public:
  Debugger(Args args)
    : m_command_interpreter_sp(new CommandInterpreter())
    , m_communicator_sp(new Communicator())
    , m_args(args)
    , m_cmake_instance_sp(new CMakeInstance())
  {
  }
  void RunMainLoop();
  void RunCommandInterpreter();
  void LaunchCMakeInstance();

  /* CommandInterpreter& GetCommandInterpreter() { return *m_command_interpreter_sp; } */
  /* CMakeInstance& GetCMakeInstance() { return *m_cmake_instance_sp; } */

private:
  CommandInterpreterSP m_command_interpreter_sp;
  CMakeInstanceSP m_cmake_instance_sp;
  CommunicatorSP m_communicator_sp;
  Args m_args;
};
