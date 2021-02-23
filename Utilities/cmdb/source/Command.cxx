#include <Command.h>
#include <CommandInterpreter.h>
#include <HLDPServer/HLDP.h>
#include <Utility.h>

#include <iostream>

void ContinueCommand::DoExecute() {
  if (!m_cmake_sp->ContinueProcess())
    (void)0;
  if (!m_cmake_sp->ListenForTargetRunning())
    (void)0;
  if (!m_cmake_sp->ListenForTargetStopped())
    (void)0;
}
void QuitCommand::DoExecute() { m_cmake_sp->KillProcess(); }
void BreakpointCommand::DoExecute() {
  std::cout << "Subcommands for breakpoint:\n"
               "    set        - set a breakpoint\n"
               "    delete     - delete a breakpoint\n"
               "    list       - list all breakpoints\n";
}

void ListCommand::DoExecute() { std::cout << "NYI\n"; }

void BreakpointSetCommand::DoExecute() {
  std::string result;
  if (m_type == Type::Function) {
    m_cmake_sp->CreateFunctionBreakpoint(m_function);
    result = m_function;
  } else {
    m_cmake_sp->CreateFileAndLineBreakpoint(m_file, m_line);
    result = m_file + ":" + std::to_string(m_line);
  }
  m_cmake_sp->ListenForBreakpointCreated(result);
}
void BreakpointSetCommand::ParseCommand(std::stack<std::string> &stk) {
  // TODO: make this parse thigns other than just the symbol
  auto first = stk.top();
  stk.pop();
  if (stk.size() == 0) {
    m_function = first;
    m_type = Type::Function;
  } else {
    m_file = first;
    m_line = std::stoi(stk.top());
    m_type = Type::FileAndLine;
  }
}

void BreakpointDeleteCommand::DoExecute() {
  std::cout << "NYI: breakpoint delete\n";
}
void BreakpointListCommand::DoExecute() { m_cmake_sp->DumpBreakpointList(); }

void WatchpointCommand::DoExecute() {
  std::cout << "Subcommands:\n"
               "    set     - set a watchpoint\n"
               "    delete  - delete a watchpoint\n"
               "    list    - list all watchpoints\n";
}
void WatchpointSetCommand::ParseCommand(std::stack<std::string> &stk) {
  while (!stk.empty()) {
    auto arg = stk.top();
    stk.pop();
    if (arg == "--target") {
      m_isTarget = true;
      continue;
    } else if (arg == "--read") {
      m_isVarAccess = true;
      continue;
    } else if (arg == "--write") {
      m_isVarUpdate = true;
      continue;
    } else {
      m_name = arg;
    }
  }
}
void WatchpointSetCommand::DoExecute() {
  std::string result;
  m_cmake_sp->CreateWatchpoint(m_isTarget, m_isVarAccess, m_isVarUpdate,
                               m_name);
  m_cmake_sp->ListenForWatchpointCreated(result);
}
void WatchpointDeleteCommand::DoExecute() {
  std::cout << "NYI: watchpoint delete\n";
}
void WatchpointListCommand::DoExecute() {
  std::cout << "NYI: watchpoint list\n";
}

void UntilCommand::DoExecute() { std::cout << "NYI: Until" << std::endl; }

void VariableCommand::DoExecute() {
  bool result;
  if (m_variable.has_value())
    result = m_cmake_sp->EvaluateVariable(*m_variable);
  else
    result = m_cmake_sp->EvaluateAllVariables();

  if (!result)
    std::cout << "Failed to evaluate variable" << std::endl;
}
void VariableCommand::ParseCommand(std::stack<std::string> &stk) {
  m_variable =
      stk.size() > 0 ? std::optional<std::string>{stk.top()} : std::nullopt;
}
void ChildrenCommand::DoExecute() { std::cout << "NYI: Children" << std::endl; }
void DetachCommand::DoExecute() { m_cmake_sp->DetachFromProcess(); }

void StepCommand::DoExecute() {
  std::cout
      << "Subcommands:\n"
         "    in        - if possible, step into the current function call\n"
         "    out       - step out of the current function\n"
         "    over      - step over the current line\n";
}

void StepInCommand::DoExecute() {
  m_cmake_sp->StepIn();
  if (!m_cmake_sp->ListenForTargetRunning())
    (void)0;
  if (!m_cmake_sp->ListenForTargetStopped())
    (void)0;
}

void StepOutCommand::DoExecute() {
  m_cmake_sp->StepOut();
  if (!m_cmake_sp->ListenForTargetRunning())
    (void)0;
  if (!m_cmake_sp->ListenForTargetStopped())
    (void)0;
}

void StepOverCommand::DoExecute() {
  m_cmake_sp->StepOver();
  if (!m_cmake_sp->ListenForTargetRunning())
    (void)0;
  if (!m_cmake_sp->ListenForTargetStopped())
    (void)0;
}

void HelpCommand::DoExecute() {
  auto command_list = CommandList::Get();
  for (auto const &command : command_list) {
    std::cout << "    " << command->GetCommandString() << " : "
              << command->GetOneLineHelp() << '\n';
  }
}

void CommandAlias::DoExecute() { assert(false); }

void AliasCommand::DoExecute() {
  std::cout << "Subcommands for alias:\n"
               "    set        - set a alias\n"
               "    delete     - delete a alias\n"
               "    list       - list all aliass\n";
}
void AliasSetCommand::DoExecute() {
  std::string result;
  auto command = m_command_interpreter_sp->ParseCommand(m_target_stack,
                                                        CommandList::Get());
  if (command.has_value()) {
    auto command_alias =
        std::make_shared<CommandAlias>(m_alias, command.value());
    CommandList::Get().push_back(command_alias);
    std::cout << "Set alias for " << m_alias << '\n';
  } else {
    std::cout << "Invalid alias target\n";
  }
}
void AliasSetCommand::ParseCommand(std::stack<std::string> &stk) {
  auto first = stk.top();
  stk.pop();
  m_alias = first;

  m_target_stack = stk;
  ;
}

void AliasDeleteCommand::DoExecute() { std::cout << "NYI: alias delete\n"; }
void AliasListCommand::DoExecute() {
  std::cout << "Command aliases: \n";
  for (auto &ele : CommandList::Get()) {
    auto alias = dynamic_cast<CommandAlias *>(ele.get());
    if (alias == nullptr)
      continue;
    std::cout << alias->GetCommandString() << " : ";
    auto target = alias->GetTargetCommand();
    auto parent = target->GetParentCommand();
    if (parent != nullptr) {
      std::cout << parent->GetCommandString() << " ";
    }
    std::cout << target->GetCommandString() << '\n';
  }
}
