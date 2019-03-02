#include <Args.h>
#include <Debugger.h>

void Debugger::RunMainLoop() {
  LaunchCMakeInstance();
  RunCommandInterpreter();
}

void Debugger::LaunchCMakeInstance() {

  m_cmake_instance_sp->Launch(m_args);
  m_communicator_sp->ConnectToCMakeInstance(*m_cmake_instance_sp);
  m_command_interpreter_sp->SetCommunicatorSP(
      m_communicator_sp->shared_from_this());
}

void Debugger::RunCommandInterpreter() {

  m_command_interpreter_sp->RunMainLoop();
}
