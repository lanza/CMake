#pragma once

#include <ForwardDeclare.h>

#include <CMakeCommunicator.h>
#include <CMakeInstance.h>
#include <CommandInterpreter.h>
#include <IDECommunicator.h>
#include <Socket.h>

#include <thread>

class Debugger {
public:
  Debugger(Args args);
  void RunMainLoop();
  void CleanUp();
  void RunCommandInterpreter();
  void LaunchCMakeInstance();
  void LaunchIDEThread();

  CommandInterpreterSP const GetCommandInterpreterSP() const {
    return m_command_interpreter_sp;
  }
  CMakeInstanceSP const GetCMakeInstanceSP() const {
    return m_cmake_instance_sp;
  }
  CMakeCommunicatorSP const GetCMakeCommunicatorSP() const {
    return m_cmake_communicator_sp;
  }

  void SetIDECommunicatorSP(IDECommunicatorSP comm) {
    m_ide_communicator_sp = comm;
  }
  IDECommunicatorSP GetIDECommunicatorSP() { return m_ide_communicator_sp; }

private:
  CommandInterpreterSP m_command_interpreter_sp;
  CMakeInstanceSP m_cmake_instance_sp;
  CMakeCommunicatorSP m_cmake_communicator_sp;
  Args m_args;

  std::thread m_ide_thread;
  IDECommunicatorSP m_ide_communicator_sp;
};
