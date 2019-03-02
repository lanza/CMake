#pragma once

#include <Communicator.h>

class Command : public std::enable_shared_from_this<Command> {
public:
//Command(std::string command_string) {
  virtual void DoExecute() = 0;

  virtual std::string const GetCommandString() const = 0;
  virtual bool HasSubcommands() const = 0;
  ~Command() {};
protected:
  std::string m_command_string;
};

class ContinueCommand : public Command {
public:
  std::string const GetCommandString() const {
    return "continue";
  }
  bool HasSubcommands() const { return false; }
  void DoExecute();
private:
};

class QuitCommand : public Command {
public:
  std::string const GetCommandString() const {
    return "quit";
  }
  bool HasSubcommands() const { return false; }
  void DoExecute();
};

class BreakpointCommand : public Command {
public:
  std::string const GetCommandString() const {
    return "breakpoint";
  }
  bool HasSubcommands() const { return true; }
  void DoExecute();
};


using CommandSP = std::shared_ptr<Command>;

