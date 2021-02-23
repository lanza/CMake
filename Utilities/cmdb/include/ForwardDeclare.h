#pragma once
#include <memory>

class CMakeInstance;
using CMakeInstanceSP = std::shared_ptr<CMakeInstance>;
class Command;
using CommandSP = std::shared_ptr<Command>;
class CommandAlias;
using CommandAliasSP = std::shared_ptr<CommandAlias>;
class CommandInterpreter;
using CommandInterpreterSP = std::shared_ptr<CommandInterpreter>;
class CMakeCommunicator;
using CMakeCommunicatorSP = std::shared_ptr<CMakeCommunicator>;
class IDECommunicator;
using IDECommunicatorSP = std::shared_ptr<IDECommunicator>;
class Debugger;
using DebuggerSP = std::shared_ptr<Debugger>;
class Socket;
using SocketUP = std::unique_ptr<Socket>;
class Socket;
using SocketUP = std::unique_ptr<Socket>;
