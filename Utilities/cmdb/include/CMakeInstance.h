#pragma once

#include <ForwardDeclare.h>

#include <Args.h>
#include <Backtrace.h>
#include <Breakpoint.h>
#include <Watchpoint.h>

#ifndef WIN32
#include <unistd.h>
#endif

#include <memory>
#include <optional>

#include <HLDPServer/HLDP.h>
#include <HLDPServer/RequestReader.h>

class CMakeInstance : public std::enable_shared_from_this<CMakeInstance> {
public:
  CMakeInstance() : m_state{State::Unknown} {}
  void Launch(Args &args);
  void RunCMake(Args &args);
  void TargetDidStop(RequestReader &reader);

  bool ContinueProcess();
  bool CreateWatchpoint(bool isTarget, bool isVarAccess, bool isVarUpdate,
                        std::string const &name);
  bool CreateFunctionBreakpoint(std::string function);
  bool CreateFileAndLineBreakpoint(std::string file, int line);
  bool KillProcess();
  bool DetachFromProcess();
  bool SetBreakpoint();
  bool GetStop();

  pid_t GetPID() { return m_pid; }

  bool ListenForTargetRunning();
  bool ListenForTargetStopped();
  bool ListenForBreakpointCreated(std::string result);
  bool ListenForWatchpointCreated(std::string result);

  bool StepOver();
  bool StepIn();
  bool StepOut();

  bool EvaluateVariable(std::string variable);
  bool EvaluateAllVariables();

  bool WaitForStop();

  void LoopOverDebugMessage(sp::HLDPPacketType &packet_type, RequestReader &rr);

  int HandleTargetStopped(RequestReader &rr);
  int HandleExpressionCreated(RequestReader &rr);
  int HandleAllVariables(RequestReader &rr);
  bool HandleOtherPacket(sp::HLDPPacketType packet_type, RequestReader &rr);

  void SetCMakeCommunicatorSP(CMakeCommunicatorSP comm_sp) {
    m_cmake_communicator_sp = comm_sp;
  }

  void DumpBreakpointList();
  void ReportStopFrame();

  bool IsStillExecuting() {
    return m_state != State::Terminated && m_state != State::Detached;
  }

  std::optional<Backtrace> GetTopFrame() {
    if (m_backtraces.size() == 0)
      return std::nullopt;
    return m_backtraces[0];
  }

  void SetHasIDE(bool val) { m_has_ide = val; }

  int GetPortNumber() const { return m_port_number; }
  void SetPortNumber(int port);

private:
#ifdef WIN32
  int m_pid;
#else
  pid_t m_pid;
#endif

  enum class State { Stopped, Running, Unknown, Terminated, Detached };
  State m_state;
  CMakeCommunicatorSP m_cmake_communicator_sp;
  std::vector<Backtrace> m_backtraces;
  std::vector<Breakpoint> m_breakpoints;
  std::vector<Watchpoint> m_watchpoints;
  bool m_has_ide = false;
  int m_port_number = 15273;
};
