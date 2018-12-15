
#include <Debugger.h>
#include <Args.h>


void Debugger::RunMainLoop() {
  LaunchCMakeInstance();
  RunCommandInterpreter();
}

void Debugger::LaunchCMakeInstance() {

  m_cmake_instance_sp->Launch(m_args);
  m_communicator.ConnectToCMakeInstance(*m_cmake_instance_sp);
}

void Debugger::RunCommandInterpreter() {

}
