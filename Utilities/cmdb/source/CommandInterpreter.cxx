#include <CommandInterpreter.h>

#include <algorithm>
#include <cstdlib>

#include <optional>

#include <Command.h>
#include <Utility.h>

void CommandInterpreter::RunMainLoop() {

  while (true) {
    std::string line = m_editline.GetLine();
    line.erase(std::remove(line.begin(), line.end(), '\n'), line.end());

    std::optional<CommandSP> command_opt = ParseCommandLine(line);
    if (command_opt.has_value()) {
      (*command_opt)->DoExecute();
    } else {
      std::cout << "No command\n";
    }
  }
}

std::optional<CommandSP>
CommandInterpreter::ParseCommandLine(std::string const &line) {

  std::vector<std::string> words = SplitString(line, ' ');
  std::string first_word = words[0];

  std::vector<CommandSP> filtered_commands{m_command_list.size()};
  auto it =
      std::copy_if(m_command_list.begin(), m_command_list.end(),
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
    // TODO(lanza): do something to feed this to completions
    return std::nullopt;
  } else if (filtered_commands.size() == 1) {
    // TODO(lanza): filter subwords
    CommandSP command = filtered_commands[0];
    if (command->HasSubcommands() && words.size() > 1) {
      // filter subcommands
      (void)3;
    }

    return filtered_commands[0]->shared_from_this();
  } else {
    return std::nullopt;
  }
}

void CommandInterpreter::RegisterCommands() {

  m_command_list.push_back(std::make_shared<ContinueCommand>());
  m_command_list.push_back(std::make_shared<QuitCommand>());
  m_command_list.push_back(std::make_shared<BreakpointCommand>());
}
