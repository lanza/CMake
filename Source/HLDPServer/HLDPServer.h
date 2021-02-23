#pragma once

#include <map>
#include <string>
#include <vector>

#include "BreakpointManager.h"
#include "ExpressionBase.h"
#include "RAIIScope.h"
#include "ReplyBuilder.h"
#include "RequestReader.h"

enum class MessageType;
class cmCommand;
class IncomingSocket;
struct cmListFileFunction;
class ExpressionBase;

namespace sp {

enum class HLDPPacketType;
enum class TargetStopReason;

using UniqueScopeID = int;
using UniqueExpressionID = int;

class HLDPServer {
public:
  friend class RAIIScope;
  HLDPServer(int tcpPort);
  ~HLDPServer();

public: // Public interface for debugged code
  bool WaitForClient();
  std::unique_ptr<RAIIScope>
  OnExecutingInitialPass(cmState::Command cmd, cmMakefile *pMakefile,
                         const cmListFileFunction &function,
                         bool &skipThisInstruction);
  void
  AdjustNextExecutedFunction(const std::vector<cmListFileFunction> &functions,
                             size_t &i);

  void OnMessageProduced(MessageType type,
                         const std::string &message);
  void OnVariableAccessed(const std::string &variable, int access_type,
                          const char *newValue, const cmMakefile *mf);
  void OnTargetCreated(cmStateEnums::TargetType type,
                       const std::string &targetName);

private:
  void HandleBreakpointRelatedCommand(HLDPPacketType type,
                                      RequestReader &reader);
  bool SendReply(HLDPPacketType packetType, const ReplyBuilder &builder);
  HLDPPacketType
  ReceiveRequest(RequestReader &reader); // Returns 'Invalid' on error
  void SendErrorPacket(std::string details);

  void ReportStopAndServeDebugRequests(TargetStopReason stopReason,
                                       unsigned intParam,
                                       const std::string &stringParam,
                                       bool *pSkipThisInstruction);

private:
  class DomainSpecificBreakpoint;

private:
  std::unique_ptr<ExpressionBase> CreateExpression(const std::string &text,
                                                   const RAIIScope &scope);

private:
  IncomingSocket *m_pSocket;
  bool m_BreakInPending = false;
  bool m_EventsReported = false;
  bool m_Detached = false;
  int m_NextOneBasedLineToExecute = 0; // Used with "Set next statement"

  std::vector<RAIIScope *> m_CallStack;
  std::map<UniqueExpressionID, std::unique_ptr<ExpressionBase>>
      m_ExpressionCache;
  BreakpointManager m_BreakpointManager;

  // Set of variables that ever had watches created. This should reduce the
  // delay when checking each variable access.
  std::set<BreakpointManager::CaseInsensitiveObjectName>
      m_WatchedVariables;

  enum {
    kNoScope = -1,
    kRootScope = -2,
  };

  UniqueScopeID m_NextScopeID = 0;
  UniqueScopeID m_EndOfStepScopeID = kNoScope;

  UniqueExpressionID m_NextExpressionID = 0;
};
} // namespace sp
