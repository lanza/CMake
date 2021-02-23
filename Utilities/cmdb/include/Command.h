#pragma once

#include <ForwardDeclare.h>

#include <Communicator.h>

#include <optional>
#include <stack>
#include <string>
#include <utility>

class Command : public std::enable_shared_from_this<Command> {
public:
  // Command(std::string command_string) {
  virtual std::string const GetOneLineHelp() const { return ""; };
  virtual void DoExecute() = 0;
  virtual void Configure(std::string user_command, CMakeInstanceSP cmake_sp) {
    m_user_command = user_command;
    m_cmake_sp = cmake_sp;
  };

  // subclasses should implement this so that it returns the first word of the
  // command
  virtual std::string const GetCommandString() const = 0;
  virtual void ParseCommand(std::stack<std::string> &stk) { (void)stk; }
  virtual std::optional<std::vector<CommandSP>> GetSubcommands() const {
    return std::nullopt;
  }
  virtual ~Command(){};

  void SetCommandInterpreterSP(CommandInterpreterSP s) {
    m_command_interpreter_sp = s;
  }
  void SetParentCommand(Command *command) { m_parent_command = command; }
  Command *GetParentCommand() const { return m_parent_command; }

protected:
  // The command entered by the user
  std::string m_user_command;
  CMakeInstanceSP m_cmake_sp;
  CommandInterpreterSP m_command_interpreter_sp;
  Command *m_parent_command;
};

class ContinueCommand : public Command {
public:
  std::string const GetOneLineHelp() const override {
    return "continues the execution of the process";
  }
  std::string const GetCommandString() const override { return "continue"; }
  void DoExecute() override;

private:
};

class ListCommand : public Command {
public:
  std::string const GetOneLineHelp() const override {
    return "prints out source code";
  }
  std::string const GetCommandString() const override { return "list"; }
  void DoExecute() override;

private:
};

class UntilCommand : public Command {
public:
  std::string const GetOneLineHelp() const override {
    return "continue execution until the line passed as an argument";
  }
  std::string const GetCommandString() const override { return "until"; }
  void DoExecute() override;

private:
};

class VariableCommand : public Command {
public:
  std::string const GetOneLineHelp() const override {
    return "evalute a cmake variable";
  }
  std::string const GetCommandString() const override { return "variable"; }
  virtual void ParseCommand(std::stack<std::string> &stk) override;
  void DoExecute() override;

private:
  std::optional<std::string> m_variable;
};

class ChildrenCommand : public Command {
public:
  std::string const GetOneLineHelp() const override {
    return "i forget what this does tbh";
  }
  std::string const GetCommandString() const override { return "children"; }
  void DoExecute() override;

private:
};

class QuitCommand : public Command {
public:
  std::string const GetOneLineHelp() const override {
    return "kills the cmake process and exits cmdb";
  }
  std::string const GetCommandString() const override { return "quit"; }
  void DoExecute() override;
};
class DetachCommand : public Command {
public:
  std::string const GetOneLineHelp() const override {
    return "detaches from the cmake process and exits cmdb";
  }
  std::string const GetCommandString() const override { return "detach"; }
  void DoExecute() override;
};

class BreakpointSetCommand : public Command {
public:
  std::string const GetOneLineHelp() const override {
    return "set a breakpoint";
  }
  std::string const GetCommandString() const override { return "set"; }
  void DoExecute() override;
  void ParseCommand(std::stack<std::string> &stk) override;

private:
  std::string m_function;
  std::string m_file;
  int m_line;
  enum class Type { Function, FileAndLine };
  Type m_type;
};

class BreakpointDeleteCommand : public Command {
public:
  std::string const GetOneLineHelp() const override {
    return "delete a breakpoint";
  }
  std::string const GetCommandString() const override { return "delete"; }
  void DoExecute() override;
};

class BreakpointListCommand : public Command {
public:
  std::string const GetOneLineHelp() const override {
    return "list all breakpoints";
  }
  std::string const GetCommandString() const override { return "list"; }
  void DoExecute() override;
};

class BreakpointCommand : public Command {
public:
  BreakpointCommand() {
    auto bp_set = new BreakpointSetCommand();
    bp_set->SetParentCommand(this);
    m_subcommands.emplace_back(bp_set);
    auto bp_delete = new BreakpointDeleteCommand();
    bp_delete->SetParentCommand(this);
    m_subcommands.emplace_back(bp_delete);
    auto bp_list = new BreakpointListCommand();
    bp_list->SetParentCommand(this);
    m_subcommands.emplace_back(bp_list);
  }
  std::optional<std::vector<CommandSP>> GetSubcommands() const override {
    return m_subcommands;
  }
  std::string const GetCommandString() const override { return "breakpoint"; }
  void DoExecute() override;

private:
  std::vector<CommandSP> m_subcommands;
};

class WatchpointSetCommand : public Command {
public:
  std::string const GetOneLineHelp() const override {
    return "set a watchpoint";
  }

  std::string m_name;
  bool m_isTarget;
  bool m_isVarAccess;
  bool m_isVarUpdate;

  void ParseCommand(std::stack<std::string> &stk) override;
  std::string const GetCommandString() const override { return "set"; }
  void DoExecute() override;
};

class WatchpointDeleteCommand : public Command {
public:
  std::string const GetOneLineHelp() const override {
    return "delete a watchpoint";
  }
  std::string const GetCommandString() const override { return "delete"; }
  void DoExecute() override;
};

class WatchpointListCommand : public Command {
public:
  std::string const GetOneLineHelp() const override {
    return "list all watchpoints";
  }
  std::string const GetCommandString() const override { return "list"; }
  void DoExecute() override;
};

class WatchpointCommand : public Command {
public:
  WatchpointCommand() {
    auto wp_set = new WatchpointSetCommand();
    wp_set->SetParentCommand(this);
    m_subcommands.emplace_back(wp_set);
    auto wp_delete = new WatchpointDeleteCommand();
    wp_delete->SetParentCommand(this);
    m_subcommands.emplace_back(wp_delete);
    auto wp_list = new WatchpointListCommand();
    wp_list->SetParentCommand(this);
    m_subcommands.emplace_back(wp_list);
  }
  std::string const GetOneLineHelp() const override {
    return "set, delete or list watchpoints";
  }
  std::optional<std::vector<CommandSP>> GetSubcommands() const override {
    return m_subcommands;
  }
  std::string const GetCommandString() const override { return "watchpoint"; }
  void DoExecute() override;

private:
  std::vector<CommandSP> m_subcommands;
};

class StepInCommand : public Command {
public:
  std::string const GetOneLineHelp() const override {
    return "step into the line of code";
  }
  std::string const GetCommandString() const override { return "in"; }
  void DoExecute() override;
};

class StepOutCommand : public Command {
public:
  std::string const GetOneLineHelp() const override {
    return "step out of the current frame";
  }
  std::string const GetCommandString() const override { return "out"; }
  void DoExecute() override;
};

class StepOverCommand : public Command {
public:
  std::string const GetOneLineHelp() const override {
    return "step over a line of code";
  }
  std::string const GetCommandString() const override { return "over"; }
  void DoExecute() override;
};

class StepCommand : public Command {
public:
  std::string const GetOneLineHelp() const override {
    return "step into, out of or over a line of code";
  }
  StepCommand() {
    auto s_in = new StepInCommand();
    s_in->SetParentCommand(this);
    m_subcommands.emplace_back(s_in);
    auto s_out = new StepOutCommand();
    s_out->SetParentCommand(this);
    m_subcommands.emplace_back(s_out);
    auto s_over = new StepOverCommand();
    s_over->SetParentCommand(this);
    m_subcommands.emplace_back(s_over);
  }
  std::optional<std::vector<CommandSP>> GetSubcommands() const override {
    return m_subcommands;
  }
  std::string const GetCommandString() const override { return "step"; }
  void DoExecute() override;

private:
  std::vector<CommandSP> m_subcommands;
};

class HelpCommand : public Command {
public:
  std::string const GetOneLineHelp() const override {
    return "the command to get help on other commands";
  }
  std::string const GetCommandString() const override { return "help"; }
  void DoExecute() override;
};

class CommandAlias : public Command {
public:
  std::string const GetOneLineHelp() const override {
    return "a wrapper for dispatching to aliased commands";
  }
  std::string const GetCommandString() const override { return m_alias; }

  CommandAlias(std::string alias, CommandSP target_command)
      : m_alias{alias}, m_target_command{target_command} {}
  void DoExecute() override;
  CommandSP GetTargetCommand() { return m_target_command; }

private:
  std::string m_alias;
  CommandSP m_target_command;
};

class AliasSetCommand : public Command {
public:
  std::string const GetOneLineHelp() const override {
    return "set a new command alias";
  }
  std::string const GetCommandString() const override { return "set"; }
  void DoExecute() override;
  void ParseCommand(std::stack<std::string> &stk) override;

private:
  std::string m_alias;
  std::stack<std::string> m_target_stack;
};

class AliasDeleteCommand : public Command {
public:
  std::string const GetOneLineHelp() const override {
    return "delete a command alias by index";
  }
  std::string const GetCommandString() const override { return "delete"; }
  void DoExecute() override;
};

class AliasListCommand : public Command {
public:
  std::string const GetOneLineHelp() const override {
    return "list all defined command aliases";
  }
  std::string const GetCommandString() const override { return "list"; }
  void DoExecute() override;
};

class AliasCommand : public Command {
public:
  AliasCommand() {
    auto a_set = new AliasSetCommand();
    a_set->SetParentCommand(this);
    m_subcommands.emplace_back(a_set);
    auto a_delete = new AliasDeleteCommand();
    a_delete->SetParentCommand(this);
    m_subcommands.emplace_back(a_delete);
    auto a_list = new AliasListCommand();
    a_list->SetParentCommand(this);
    m_subcommands.emplace_back(a_list);
  }
  std::string const GetOneLineHelp() const override {
    return "set, delete or list command aliases";
  }
  std::optional<std::vector<CommandSP>> GetSubcommands() const override {
    return m_subcommands;
  }
  const std::string GetCommandString() const override { return "alias"; }
  void DoExecute() override;

private:
  std::vector<CommandSP> m_subcommands;
};

static std::vector<CommandSP> g_command_list;
class CommandList {
public:
  static std::vector<CommandSP> &Get() {
    if (g_command_list.size() == 0) {
#define ADD_COMMAND(name)                                                      \
  auto name = std::make_shared<name##Command>();                               \
  g_command_list.push_back(name)
      ADD_COMMAND(Continue);
      ADD_COMMAND(Quit);
      ADD_COMMAND(Breakpoint);
      ADD_COMMAND(List);
      ADD_COMMAND(Detach);
      ADD_COMMAND(Until);
      ADD_COMMAND(Watchpoint);
      ADD_COMMAND(Step);
      ADD_COMMAND(Variable);
      ADD_COMMAND(Children);
      ADD_COMMAND(Help);
      ADD_COMMAND(Alias);
#define ADD_ALIAS(alias, target)                                               \
  g_command_list.push_back(                                                    \
      std::move(std::make_shared<CommandAlias>(alias, target)));

      auto break_subcommands = Breakpoint->GetSubcommands().value();
      auto BreakpointSet = break_subcommands[0];
      ADD_ALIAS("b", BreakpointSet);

      auto step_subcommands = Step->GetSubcommands().value();
      auto StepIn = step_subcommands[0];
      ADD_ALIAS("s", StepIn);
      auto StepOut = step_subcommands[1];
      ADD_ALIAS("o", StepOut);
      auto StepOver = step_subcommands[2];
      ADD_ALIAS("n", StepOver);
      ADD_ALIAS("exit", Quit);

      ADD_ALIAS("c", Continue);
    }
    return g_command_list;
  }
};
