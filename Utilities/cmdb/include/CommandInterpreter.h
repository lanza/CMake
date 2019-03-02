#pragma once

#include <Editline.h>
#include <Command.h>
#include <Communicator.h>

#include <optional>
#include <vector>

class CommandInterpreter: std::enable_shared_from_this<CommandInterpreter> {
public:
  CommandInterpreter() :m_editline() {
    RegisterCommands();
  }

  void SetCommunicatorSP(CommunicatorSP communicator_sp) {
    m_communicator_sp = communicator_sp;
  }

  void RunMainLoop();
  std::optional<CommandSP> ParseCommandLine(std::string const& line);

  void RegisterCommands();

private:
  Editline m_editline;
  CommunicatorSP m_communicator_sp;
  std::vector<CommandSP> m_command_list;
};

using CommandInterpreterSP = std::shared_ptr<CommandInterpreter>;
