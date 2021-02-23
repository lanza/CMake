#pragma once

#include <ForwardDeclare.h>

#include <Command.h>
#include <Communicator.h>
#include <Debugger.h>
#include <Editline.h>

#include <optional>
#include <stack>
#include <string>
#include <vector>

class CommandInterpreter
    : public std::enable_shared_from_this<CommandInterpreter> {
public:
  CommandInterpreter() : m_editline(), m_command_list{CommandList::Get()} {}

  void SetCMakeCommunicatorSP(CMakeCommunicatorSP cmake_communicator_sp) {
    m_cmake_communicator_sp = cmake_communicator_sp;
  }

  void RunMainLoop();
  std::optional<CommandSP>
  ParseCommand(std::stack<std::string> &line,
               std::vector<CommandSP> const &command_list);

  Debugger const *GetDebugger() const { return m_debugger; }
  void SetDebugger(Debugger *debugger) { m_debugger = debugger; }

private:
  Debugger *m_debugger;
  Editline m_editline;
  CMakeCommunicatorSP m_cmake_communicator_sp;
  std::vector<CommandSP> &m_command_list;
};
