#include <CommandInterpreter.h>

#include <algorithm>
#include <cstdlib>

#include <iterator>
#include <optional>
#include <stack>
#include <string>

#include <Command.h>
#include <Utility.h>

using PT = sp::HLDPPacketType;

void CommandInterpreter::RunMainLoop() {

  std::string last_command;

  auto ide_comm_sp = m_debugger->GetIDECommunicatorSP();
  auto cmake_instance_sp = m_debugger->GetCMakeInstanceSP();

  while (true && cmake_instance_sp->IsStillExecuting()) {
    if (ide_comm_sp) {
      auto bt = cmake_instance_sp->GetTopFrame();
      if (bt.has_value()) {
        ReplyBuilder b;
        auto value = bt.value();
        b.AppendString(value.m_filename);
        b.AppendInt32(value.m_line);
        ide_comm_sp->SendPacket(PT::ciUpdateFrame, &b);
      }
    }
    std::string line = m_editline.GetLine();
    line.erase(std::remove(line.begin(), line.end(), '\n'), line.end());
    if (line.size() == 0) {
      line = last_command;
    }

    auto words = SplitString(line, ' ');

    std::stack<std::string> stk;
    for (auto word = words.rbegin(); word != words.rend(); ++word)
      stk.push(*word);

    std::optional<CommandSP> command_opt = ParseCommand(stk, m_command_list);
    if (command_opt.has_value()) {
      auto command = *command_opt;
      auto alias = dynamic_cast<CommandAlias *>(command.get());
      if (alias != nullptr)
        command = alias->GetTargetCommand();
      command->Configure(line, m_debugger->GetCMakeInstanceSP());
      command->SetCommandInterpreterSP(this->shared_from_this());
      command->ParseCommand(stk);
      command->DoExecute();
      last_command = line;
    } else {
      std::cout << "error: '" << line << "' is not a valid command."
                << std::endl;
    }
  }
}

std::optional<CommandSP>
CommandInterpreter::ParseCommand(std::stack<std::string> &stk,
                                 std::vector<CommandSP> const &command_list) {

  if (stk.size() == 0)
    return std::nullopt;
  std::string first_word = stk.top();
  stk.pop();

  std::vector<CommandSP> filtered_commands{command_list.size()};
  auto it =
      std::copy_if(command_list.begin(), command_list.end(),
                   filtered_commands.begin(), [&](CommandSP const command_sp) {
                     if (command_sp->GetCommandString().compare(
                             0, first_word.size(), first_word) == 0) {
                       return true;
                     } else {
                       return false;
                     }
                   });
  filtered_commands.resize(std::distance(filtered_commands.begin(), it));

  if (filtered_commands.size() > 1) {
    // Check if it's an absolute match -- e.g. c matches c, continue, children
    // but is an absolute match for c
    for (auto &filtered_command : filtered_commands) {
      if (first_word == filtered_command->GetCommandString())
        return filtered_command;
    }
    std::cout << "Possible matching commands: " << std::endl;
    for (auto const &command : filtered_commands) {
      std::cout << "    " << command->GetCommandString() << '\n';
    }
    return std::nullopt;
  }
  if (filtered_commands.size() == 0)
    return std::nullopt;

  CommandSP command = filtered_commands[0];
  if (stk.size() > 0) {
    if (auto subcommands = command->GetSubcommands()) {
      return ParseCommand(stk, subcommands.value());
    }
  }
  return filtered_commands[0];
}
