#include <Args.h>
#include <Backtrace.h>
#include <CMakeCommunicator.h>
#include <CMakeInstance.h>

#ifndef WIN32
#include <unistd.h>
#endif

#include <array>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include <cstring>

extern int SOCK;

namespace fs = std::filesystem;

static std::optional<std::string> exec(const char *cmd) {
  std::array<char, 128> buffer;
  std::string result;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
  if (!pipe)
    return std::nullopt;
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
    result += buffer.data();

  // remove newline
  result.pop_back();
  return result;
}

static std::string GetCMakePath(Args const &args) {
  auto argv = args.GetArgv();
  std::string binary = argv[0];
  auto pos = binary.find("cmdb");
  if (pos == std::string::npos) {
    std::cerr << "Figure out how to handle non / paths to cmdb\n";
    exit(45);
  }
  auto sub = binary.replace(pos, binary.length(), "cmake");

  if (binary[0] == '/' && fs::exists(sub))
    return sub;

  fs::path p = fs::current_path();
  p.append(binary);

  auto permis = fs::status(p).permissions();
  if (fs::exists(p) && fs::is_regular_file(p) &&
      ((fs::status(p).permissions() & fs::perms::others_exec) ==
       fs::perms::others_exec))
    return p;

  auto result = exec("which cmake");
  if (!result)
    std::cerr << "can't find cmake\n";

  return result.value();
}

void CMakeInstance::RunCMake(Args &args) {
  // +2 to account for "--debug-server-port", the port and the NULL
  // while subtracing the executable name
  char const **new_argv = new char const *[args.GetArgc() + 2];
  auto original_argv = args.GetArgv();
  auto original_argc = args.GetArgc();

  int j = 1;
  for (int i = 1; i < original_argc; i++) {
    if (strcmp(original_argv[i], "-m") == 0) {
      j--;
    } else {
      new_argv[i + j] = original_argv[i];
    }
  }

  auto path = GetCMakePath(args);
  new_argv[0] = path.c_str();

  std::string argv1 = "--debug-server-port=";
  argv1 += std::to_string(this->m_port_number);

  new_argv[1] = argv1.c_str();
  new_argv[original_argc + j] = NULL;

  for (int i = 0; i < original_argc + j; i++) {
    std::cout << new_argv[i] << std::endl;
  }
#ifndef WIN32
  execv(path.c_str(), (char **)new_argv);
#endif
}

void CMakeInstance::SetPortNumber(int port) {
  if (port <= 0) {
    srand(time(0));
    port = std::rand();
    if (port < 2000)
      port += 2000;
  }
  this->m_port_number = port;
}

void CMakeInstance::Launch(Args &args) {

#ifndef WIN32
  m_pid = fork();
  if (m_pid == 0) {
    RunCMake(args);
  } else {
    // TODO(lanza): implement this
  }
#endif
}

void CMakeInstance::LoopOverDebugMessage(sp::HLDPPacketType &packet_type,
                                         RequestReader &rr) {
  while (packet_type == sp::HLDPPacketType::scDebugMessage) {
    int strm;
    auto result = rr.ReadInt32(&strm);
    if (!result)
      (void)result; // fail?

    std::string message;
    result = rr.ReadString(&message);
    if (!result)
      (void)result; // fail?

    packet_type = m_cmake_communicator_sp->ReceivePacket(rr);
  }
}

bool CMakeInstance::ListenForBreakpointCreated(std::string result) {
  RequestReader rr;
  sp::HLDPPacketType packet_type = m_cmake_communicator_sp->ReceivePacket(rr);

  if (packet_type == sp::HLDPPacketType::scBreakpointCreated) {
    int id;
    rr.ReadInt32(&id);
    std::cout << "Breakpoint created: " << id << " - " << result << std::endl;
    m_breakpoints.emplace_back(id, result);
    return true;
  } else {
    HandleOtherPacket(packet_type, rr);
    return false;
  }
}
bool CMakeInstance::ListenForWatchpointCreated(std::string result) {
  RequestReader rr;
  sp::HLDPPacketType packet_type = m_cmake_communicator_sp->ReceivePacket(rr);

  if (packet_type == sp::HLDPPacketType::scBreakpointCreated) {
    int id;
    rr.ReadInt32(&id);
    std::cout << "Watchpoing created: " << id << " - " << result << std::endl;
    m_watchpoints.emplace_back(id, result);
    return true;
  } else {
    HandleOtherPacket(packet_type, rr);
    return false;
  }
}
bool CMakeInstance::ListenForTargetRunning() {
  RequestReader rr;
  sp::HLDPPacketType packet_type = m_cmake_communicator_sp->ReceivePacket(rr);

  LoopOverDebugMessage(packet_type, rr);

  if (packet_type == sp::HLDPPacketType::scTargetRunning) {
    m_state = State::Running;
    return true;
  } else {
    HandleOtherPacket(packet_type, rr);
    return false;
  }
}

bool CMakeInstance::ListenForTargetStopped() {
  RequestReader rr;
  sp::HLDPPacketType packet_type = m_cmake_communicator_sp->ReceivePacket(rr);
  LoopOverDebugMessage(packet_type, rr);

  if (packet_type == sp::HLDPPacketType::scTargetStopped) {
    m_state = State::Stopped;
    HandleTargetStopped(rr);
    return true;
  } else {
    auto result = HandleOtherPacket(packet_type, rr);
    return false;
  }
}

int CMakeInstance::HandleAllVariables(RequestReader &rr) {
  int size;
  rr.ReadInt32(&size);

  for (int i = 0; i < size; ++i) {
    if (!HandleExpressionCreated(rr))
      return false;
  }

  return true;
}

int CMakeInstance::HandleExpressionCreated(RequestReader &rr) {
  int int_arg;
  std::string string_arg;

  rr.ReadInt32(&int_arg);
  int id = int_arg;
  rr.ReadString(&string_arg);
  auto name = string_arg;
  rr.ReadString(&string_arg);
  auto type = string_arg;
  rr.ReadString(&string_arg);
  auto value = string_arg;
  rr.ReadInt32(&int_arg);
  auto flags = int_arg;
  rr.ReadInt32(&int_arg);
  auto child_count = int_arg;

  if (type == "(CMake Expression)") {
    std::cout << "ID: " << id << " - " << name << " = " << value << '\n';
  } else if (type == "(CMake target)") {
    std::cout << "ID: " << id << " - "
              << "target"
              << " = " << value << '\n';
  } else {
    std::cout << "name: " << name << '\n';
    std::cout << "value: " << value << '\n';
    std::cout << "type: " << type << '\n';
    std::cout << "flags: " << flags << '\n';
    std::cout << "child_count: " << child_count << '\n';
  }
  return true;
}

int CMakeInstance::HandleTargetStopped(RequestReader &rr) {
  int int_arg;
  std::string string_arg;
  rr.ReadInt32(&int_arg);
  sp::TargetStopReason stop_reason{int_arg};

  rr.ReadInt32(&int_arg);
  rr.ReadString(&string_arg);
  rr.ReadInt32(&int_arg);

  auto backtrace_entry_count = int_arg;

  m_backtraces.clear();
  for (int i = 0; i < backtrace_entry_count; ++i) {
    rr.ReadInt32(&int_arg);
    auto index = int_arg;
    rr.ReadString(&string_arg);
    auto idk = string_arg;
    rr.ReadString(&string_arg);
    auto args = string_arg;
    rr.ReadString(&string_arg);
    auto file = string_arg;
    rr.ReadInt32(&int_arg);
    auto line = int_arg;

    m_backtraces.emplace_back(index, idk, args, file, line);
  }

  if (stop_reason == sp::TargetStopReason::Breakpoint) {
    (void)0;
  }

  if (!m_has_ide)
    ReportStopFrame();

  // TODO: talk to vim client
  return backtrace_entry_count - 1;
}
bool CMakeInstance::HandleOtherPacket(sp::HLDPPacketType packet_type,
                                      RequestReader &rr) {
  return false;
}

void CMakeInstance::TargetDidStop(RequestReader &reader) {}

bool CMakeInstance::CreateWatchpoint(bool isTarget, bool isVarAccess,
                                     bool isVarUpdate,
                                     std::string const &name) {
  std::cout << name << std::endl;
  ReplyBuilder rb;
  if (isTarget) {
    rb.AppendInt32(
        as_integer(sp::CMakeDomainSpecificBreakpointType::TargetCreated));
  } else if (isVarAccess) {
    rb.AppendInt32(
        as_integer(sp::CMakeDomainSpecificBreakpointType::VariableAccessed));
  } else if (isVarUpdate) {
    rb.AppendInt32(
        as_integer(sp::CMakeDomainSpecificBreakpointType::VariableUpdated));
  } else {
    assert(false && "Invalid watchpoint");
  }

  rb.AppendString(name);
  // What is this for? I do it in cmdb for some reason
  rb.AppendInt32(0);

  m_cmake_communicator_sp->SendPacket(
      sp::HLDPPacketType::csCreateDomainSpecificBreakpoint, &rb);
  return true;
}
bool CMakeInstance::CreateFunctionBreakpoint(std::string function) {
  ReplyBuilder rb;
  rb.AppendString(function);
  m_cmake_communicator_sp->SendPacket(
      sp::HLDPPacketType::csCreateFunctionBreakpoint, &rb);
  return true;
}
bool CMakeInstance::CreateFileAndLineBreakpoint(std::string file, int line) {
  ReplyBuilder rb;
  rb.AppendString(file);
  rb.AppendInt32(line);
  m_cmake_communicator_sp->SendPacket(sp::HLDPPacketType::csCreateBreakpoint,
                                      &rb);
  return true;
}

bool CMakeInstance::ContinueProcess() {
  m_cmake_communicator_sp->SendPacket(sp::HLDPPacketType::csContinue, nullptr);
  return true;
}

bool HandleDebugMessage(RequestReader const &rr) { return true; }
bool HandleTargetRunning(RequestReader const &rr) { return true; }

bool CMakeInstance::WaitForStop() {
  RequestReader rr;
  auto packet_type = m_cmake_communicator_sp->ReceivePacket(rr);

  while (true) {
    switch (packet_type) {
    case sp::HLDPPacketType::scDebugMessage:
      HandleDebugMessage(rr);
    case sp::HLDPPacketType::scTargetRunning:
      HandleTargetRunning(rr);
    case sp::HLDPPacketType::scTargetStopped:
      return HandleTargetStopped(rr);
    default:
      printf("ERROR: unknown response from cmake");
      printf("cmdb might be out of sync");
      return false;
    }
  }
}

bool CMakeInstance::KillProcess() {
  m_cmake_communicator_sp->SendPacket(sp::HLDPPacketType::csTerminate, nullptr);
  m_state = State::Terminated;
  return true;
}

bool CMakeInstance::DetachFromProcess() {
  m_cmake_communicator_sp->SendPacket(sp::HLDPPacketType::csDetach, nullptr);
  m_state = State::Detached;
  return true;
}

bool CMakeInstance::SetBreakpoint() { return false; }
bool CMakeInstance::GetStop() { return false; }

void CMakeInstance::DumpBreakpointList() {
  for (int i = 0; i < m_breakpoints.size(); ++i) {
    auto &b = m_breakpoints[i];
    std::cout << i << ": ";
    b.Dump();
  }
}

void CMakeInstance::ReportStopFrame() {
  if (m_backtraces.size() == 0)
    return;

  auto &last = *m_backtraces.begin();
  last.Dump();
}

bool CMakeInstance::EvaluateAllVariables() {
  ReplyBuilder rb;
  rb.AppendInt32(m_backtraces.size() - 1);
  m_cmake_communicator_sp->SendPacket(sp::HLDPPacketType::csGetAllVariables,
                                      &rb);

  RequestReader rr;
  auto packet_type = m_cmake_communicator_sp->ReceivePacket(rr);

  if (packet_type == sp::HLDPPacketType::scAllVariables) {
    HandleAllVariables(rr);
    return true;
  } else {
    HandleOtherPacket(packet_type, rr);
    return false;
  }
}

bool CMakeInstance::EvaluateVariable(std::string variable) {
  ReplyBuilder rb;
  rb.AppendInt32(m_backtraces.size() - 1);
  rb.AppendString(variable);
  m_cmake_communicator_sp->SendPacket(sp::HLDPPacketType::csCreateExpression,
                                      &rb);

  RequestReader rr;
  auto packet_type = m_cmake_communicator_sp->ReceivePacket(rr);

  if (packet_type == sp::HLDPPacketType::scExpressionCreated) {
    HandleExpressionCreated(rr);
    return true;
  } else {
    HandleOtherPacket(packet_type, rr);
    return false;
  }
}

bool CMakeInstance::StepOver() {
  m_cmake_communicator_sp->SendPacket(sp::HLDPPacketType::csStepOver, nullptr);
  return true;
}
bool CMakeInstance::StepIn() {
  m_cmake_communicator_sp->SendPacket(sp::HLDPPacketType::csStepIn, nullptr);
  return true;
}
bool CMakeInstance::StepOut() {
  m_cmake_communicator_sp->SendPacket(sp::HLDPPacketType::csStepOut, nullptr);
  return true;
}
