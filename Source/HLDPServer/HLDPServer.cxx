#include "cmCommand.h"
#include "cmMakefile.h"
#include <iostream>

#include "ExpressionBase.h"
#include "HLDP.h"
#include "HLDPServer.h"
#include "IncomingSocket.h"
#include "RAIIScope.h"
#include "ReplyBuilder.h"
#include "RequestReader.h"

#include "cmMessageType.h"
#include "cmState.h"
#include "cmStatePrivate.h"
#include "cmSystemTools.h"
#include "cmVariableWatch.h"
#include "cmsys/String.h"
#include <cmake.h>

#include <cassert>

#include <glog/logging.h>

namespace sp {

class HLDPServer::DomainSpecificBreakpoint
    : public BreakpointManager::DomainSpecificBreakpointExtension {
public:
  CMakeDomainSpecificBreakpointType Type;
  std::string StringArg;

  DomainSpecificBreakpoint(const std::string &stringArg, int intArg)
      : Type((CMakeDomainSpecificBreakpointType)intArg), StringArg(stringArg) {}
};

HLDPServer::HLDPServer(int tcpPort) : m_BreakInPending(true) {
  google::InitGoogleLogging("cmake");
  m_pSocket = new IncomingSocket(tcpPort);
}

HLDPServer::~HLDPServer() {
  ReplyBuilder builder;
  builder.AppendInt32(0);
  SendReply(HLDPPacketType::scTargetExited, builder);
  delete m_pSocket;
}

bool HLDPServer::WaitForClient() {
  if (!m_pSocket->Accept())
    return false;

  LOG(INFO) << "WaitForClient write HLDPBanner -- size: " << sizeof(HLDPBanner);
  m_pSocket->Write(HLDPBanner, sizeof(HLDPBanner));
  ReplyBuilder builder;
  builder.AppendInt32(HLDPVersion);
  builder.AppendString("$->");
  std::string s;
  for (auto c : builder.GetBuffer())
    s += c;

  LOG(INFO) << "SendReply: " << s;
  if (!SendReply(HLDPPacketType::scHandshake, builder))
    return false;

  RequestReader reader;

  if (ReceiveRequest(reader) != HLDPPacketType::csHandshake) {
    cmSystemTools::Error("Failed to complete HLDP.handshake.");
    return false;
  }

  return true;
}

std::unique_ptr<RAIIScope>
HLDPServer::OnExecutingInitialPass(cmState::Command cmd, cmMakefile *pMakefile,
                                   const cmListFileFunction &function,
                                   bool &skipThisInstruction) {
  skipThisInstruction = false;
  if (m_Detached)
    return nullptr;

  std::unique_ptr<RAIIScope> pScope(
      new RAIIScope(this, cmd, pMakefile, function));
  struct {
    TargetStopReason reason = TargetStopReason::UnspecifiedEvent;
    unsigned intParam = 0;
    std::string stringParam;
  } stopRecord;

  {
    auto *pBreakpoint = m_BreakpointManager.TryGetBreakpointAtLocation(
        pScope->SourceFile, pScope->Function.Line());
    if (!pBreakpoint)
      pBreakpoint = m_BreakpointManager.TryGetBreakpointForFunction(
          function.OriginalName());

    if (pBreakpoint && pBreakpoint->IsEnabled) {
      m_BreakInPending = true;
      stopRecord.intParam = pBreakpoint->AssignedID;
      stopRecord.reason = TargetStopReason::Breakpoint;
    }
  }

  UniqueScopeID parentScope = kRootScope;
  if (m_CallStack.size() >= 2) {
    parentScope = m_CallStack[m_CallStack.size() - 2]->GetUniqueID();
  }

  if (parentScope == m_EndOfStepScopeID) {
    m_BreakInPending = true;
    if (stopRecord.reason == TargetStopReason::UnspecifiedEvent)
      stopRecord.reason = TargetStopReason::StepComplete;
  }

  if (!m_BreakInPending) {
    if (m_pSocket->HasIncomingData()) {
      RequestReader reader;
      HLDPPacketType requestType = ReceiveRequest(reader);
      switch (requestType) {
      case HLDPPacketType::Invalid:
        return nullptr;
      case HLDPPacketType::csBreakIn:
        stopRecord.reason = TargetStopReason::BreakInRequested;
        m_BreakInPending = true;
        break;
      default:
        if (requestType > HLDPPacketType::BeforeFirstBreakpointRelatedCommand &&
            requestType < HLDPPacketType::AfterLastBreakpointRelatedCommand) {
          HandleBreakpointRelatedCommand(requestType, reader);
        } else
          SendErrorPacket(
              "Unexpected packet received while the target is running");
        return nullptr;
      }
    }

    return pScope;
  }

  if (m_NextOneBasedLineToExecute &&
      stopRecord.reason == TargetStopReason::UnspecifiedEvent)
    stopRecord.reason = TargetStopReason::SetNextStatement;

  if (!m_EventsReported &&
      stopRecord.reason == TargetStopReason::UnspecifiedEvent)
    stopRecord.reason = TargetStopReason::InitialBreakIn;

  m_EventsReported = true;
  ReportStopAndServeDebugRequests(stopRecord.reason, stopRecord.intParam,
                                  stopRecord.stringParam, &skipThisInstruction);
  return pScope;
}

void HLDPServer::AdjustNextExecutedFunction(
    const std::vector<cmListFileFunction> &functions, size_t &i) {
  if (m_NextOneBasedLineToExecute) {
    for (int j = 0; j < functions.size(); j++) {
      if (functions[j].Line() == m_NextOneBasedLineToExecute) {
        i = j;
        return;
      }
    }
  }
}

void HLDPServer::OnMessageProduced(MessageType type,
                                   const std::string &message) {

  ReplyBuilder builder;
  builder.AppendInt32(0);
  builder.AppendString(message);
  SendReply(HLDPPacketType::scDebugMessage, builder);

  switch (type) {
  case MessageType::FATAL_ERROR:
  case MessageType::INTERNAL_ERROR:
  case MessageType::AUTHOR_ERROR:
  case MessageType::DEPRECATION_ERROR:
    ReportStopAndServeDebugRequests(TargetStopReason::Exception, 0, message,
                                    nullptr);
    return;
  default:
    break;
  }

  auto id = m_BreakpointManager.TryLocateEnabledDomainSpecificBreakpoint(
      [&](BreakpointManager::DomainSpecificBreakpointExtension *pBp) {
        switch (static_cast<DomainSpecificBreakpoint *>(pBp)->Type) {
        case CMakeDomainSpecificBreakpointType::MessageSent:
          if (message.find(
                  static_cast<DomainSpecificBreakpoint *>(pBp)->StringArg) !=
              std::string::npos)
            return true;
          else {
            return false;
          }
          break;
        default:
          return false;
        }
      });

  if (id)
    ReportStopAndServeDebugRequests(TargetStopReason::Breakpoint, id, "",
                                    nullptr);
}

void HLDPServer::OnVariableAccessed(const std::string &variable,
                                    int access_type, const char *newValue,
                                    const cmMakefile *mf) {
  if (m_WatchedVariables.find(variable) == m_WatchedVariables.end())
    return;

  bool isRead = access_type == cmVariableWatch::VARIABLE_READ_ACCESS ||
                access_type == cmVariableWatch::UNKNOWN_VARIABLE_READ_ACCESS;

  auto id = m_BreakpointManager.TryLocateEnabledDomainSpecificBreakpoint(
      [&](BreakpointManager::DomainSpecificBreakpointExtension *pBp) {
        switch (static_cast<DomainSpecificBreakpoint *>(pBp)->Type) {
        case CMakeDomainSpecificBreakpointType::VariableAccessed:
        case CMakeDomainSpecificBreakpointType::VariableUpdated:
          if (isRead == (static_cast<DomainSpecificBreakpoint *>(pBp)->Type ==
                         CMakeDomainSpecificBreakpointType::VariableAccessed) &&
              variable ==
                  static_cast<DomainSpecificBreakpoint *>(pBp)->StringArg)
            return true;
          break;
        default:
          assert(false);
          return false;
        }

        return false;
      });

  if (id)
    ReportStopAndServeDebugRequests(TargetStopReason::Breakpoint, id, "",
                                    nullptr);
}

void HLDPServer::OnTargetCreated(cmStateEnums::TargetType type,
                                 const std::string &targetName) {
  auto id = m_BreakpointManager.TryLocateEnabledDomainSpecificBreakpoint(
      [&](BreakpointManager::DomainSpecificBreakpointExtension *pBp) {
        switch (static_cast<DomainSpecificBreakpoint *>(pBp)->Type) {
        case CMakeDomainSpecificBreakpointType::TargetCreated:
          if (static_cast<DomainSpecificBreakpoint *>(pBp)->StringArg.empty() ||
              targetName ==
                  static_cast<DomainSpecificBreakpoint *>(pBp)->StringArg) {
            return true;
          } else {
            return false;
          }
          break;
        default:
          return false;
        }
      });

  if (id)
    ReportStopAndServeDebugRequests(TargetStopReason::Breakpoint, id, "",
                                    nullptr);
}

void HLDPServer::HandleBreakpointRelatedCommand(HLDPPacketType type,
                                                RequestReader &reader) {
  std::string file, stringArg;
  int line, intArg1, intArg2;
  ReplyBuilder builder;
  BreakpointManager::UniqueBreakpointID id;

  struct {
    BreakpointField field;
    int IntArg1, IntArg2;
    std::string StringArg;
  } breakpointUpdateRequest;

  switch (type) {
  case HLDPPacketType::csCreateBreakpoint:
    if (!reader.ReadString(&file) || !reader.ReadInt32(&line))
      SendErrorPacket("Invalid breakpoint request");
    else {
      id = m_BreakpointManager.CreateBreakpoint(file, line);
      if (!id)
        SendErrorPacket("Invalid or non-existent file: " + file);
      else {
        builder.AppendInt32(id);
        SendReply(HLDPPacketType::scBreakpointCreated, builder);
      }
    }
    break;
  case HLDPPacketType::csCreateFunctionBreakpoint:
    if (!reader.ReadString(&stringArg))
      SendErrorPacket("Invalid breakpoint request");
    else {
      id = m_BreakpointManager.CreateBreakpoint(stringArg);
      if (!id)
        SendErrorPacket("Failed to create a function breakpoint for " +
                        stringArg);
      else {
        builder.AppendInt32(id);
        SendReply(HLDPPacketType::scBreakpointCreated, builder);
      }
    }
    break;
  case HLDPPacketType::csCreateDomainSpecificBreakpoint:
    if (!reader.ReadInt32(&intArg1) || !reader.ReadString(&stringArg) ||
        !reader.ReadInt32(&intArg2))
      SendErrorPacket("Invalid breakpoint request");
    else {
      id = m_BreakpointManager.CreateDomainSpecificBreakpoint(
          std::make_unique<DomainSpecificBreakpoint>(stringArg, intArg1));
      if (!id)
        SendErrorPacket("Failed to create a CMake breakpoint");
      else {
        if (intArg1 ==
                (int)CMakeDomainSpecificBreakpointType::VariableAccessed ||
            intArg1 == (int)CMakeDomainSpecificBreakpointType::VariableUpdated)
          m_WatchedVariables.insert(stringArg);

        builder.AppendInt32(id);
        SendReply(HLDPPacketType::scBreakpointCreated, builder);
      }
    }
    break;
  case HLDPPacketType::csDeleteBreakpoint:
    if (!reader.ReadInt32(&id))
      SendErrorPacket("Invalid breakpoint request");
    else {
      m_BreakpointManager.DeleteBreakpoint(id);
      SendReply(HLDPPacketType::scBreakpointUpdated, builder);
    }
    break;
  case HLDPPacketType::csUpdateBreakpoint:
    if (!reader.ReadInt32(&id) ||
        !reader.ReadInt32((int *)&breakpointUpdateRequest.field) ||
        !reader.ReadInt32(&breakpointUpdateRequest.IntArg1) ||
        !reader.ReadInt32(&breakpointUpdateRequest.IntArg2) ||
        !reader.ReadString(&breakpointUpdateRequest.StringArg))
      SendErrorPacket("Invalid breakpoint request");
    else {
      auto *pBreakpoint = m_BreakpointManager.TryLookupBreakpointObject(id);
      if (!pBreakpoint)
        SendErrorPacket("Could not find a breakpoint with the specified ID");
      else {
        switch (breakpointUpdateRequest.field) {
        case BreakpointField::IsEnabled:
          pBreakpoint->IsEnabled = breakpointUpdateRequest.IntArg1 != 0;
          SendReply(HLDPPacketType::scBreakpointUpdated, builder);
          break;
        default:
          SendErrorPacket("Invalid breakpoint field");
        }
      }
    }
  default:
    assert(false);
    return;
  }
}

bool HLDPServer::SendReply(HLDPPacketType packetType,
                           const ReplyBuilder &builder) {
  HLDPPacketHeader hdr = {(unsigned)packetType,
                          (unsigned)builder.GetBuffer().size()};
  static int failCount = 0;

  LOG(INFO) << "SendReply write reply header";
  if (!m_pSocket->Write(&hdr, sizeof(hdr))) {
    cmSystemTools::Error("Failed to write debug protocol reply header.");
    cmSystemTools::SetFatalErrorOccured();
    failCount++;
    if (failCount > 3)
      _Exit(99);
    return false;
  }

  failCount = 0;

  LOG(INFO) << "SendReply write reply data: "
            << std::string{builder.GetBuffer().data()};
  if (!m_pSocket->Write(builder.GetBuffer().data(), hdr.PayloadSize)) {
    cmSystemTools::Error("Failed to write debug protocol reply payload.");
    cmSystemTools::SetFatalErrorOccured();
    failCount++;
    return false;
  }

  return true;
}

HLDPPacketType HLDPServer::ReceiveRequest(RequestReader &reader) {
  HLDPPacketHeader hdr;
  LOG(INFO) << "ReadAll packet header";
  if (!m_pSocket->ReadAll(&hdr, sizeof(hdr))) {
    cmSystemTools::Error("Failed to receive debug protocol request header.");
    cmSystemTools::SetFatalErrorOccured();
    return HLDPPacketType::Invalid;
  }

  void *pBuffer = reader.Reset(hdr.PayloadSize);
  LOG(INFO) << "ReadAll packet body start";
  if (hdr.PayloadSize != 0) {
    if (!m_pSocket->ReadAll(pBuffer, hdr.PayloadSize)) {
      cmSystemTools::Error("Failed to receive debug protocol request payload.");
      cmSystemTools::SetFatalErrorOccured();
      return HLDPPacketType::Invalid;
    }
    LOG(INFO) << "ReadAll packet body done";
  }

  return (HLDPPacketType)hdr.Type;
}

void HLDPServer::SendErrorPacket(std::string details) {
  ReplyBuilder builder;
  builder.AppendString(details);
  SendReply(HLDPPacketType::scError, builder);
}

void HLDPServer::ReportStopAndServeDebugRequests(TargetStopReason stopReason,
                                                 unsigned intParam,
                                                 const std::string &stringParam,
                                                 bool *pSkipThisInstruction) {
  m_BreakInPending = false;
  m_EndOfStepScopeID = 0;
  m_NextOneBasedLineToExecute = 0;

  ReplyBuilder builder;
  builder.AppendInt32((unsigned)stopReason);
  builder.AppendInt32(intParam);
  builder.AppendString(stringParam);

  auto backtraceEntryCount = builder.AppendDelayedInt32();

  for (int i = m_CallStack.size() - 1; i >= 0; i--) {
    auto *pEntry = m_CallStack[i];
    builder.AppendInt32(i);

    std::string args;
    builder.AppendString(m_CallStack[i]->Function.OriginalName());

    for (const auto &arg : m_CallStack[i]->Function.Arguments()) {
      if (args.length() > 0)
        args.append(", ");

      args.append(arg.Value);
    }

    builder.AppendString(args);
    builder.AppendString(pEntry->SourceFile);
    builder.AppendInt32(pEntry->Function.Line());
    (*backtraceEntryCount)++;
  }

  if (!SendReply(HLDPPacketType::scTargetStopped, builder))
    return;

  int ID = 0;
  std::string expression;

  for (;;) {
    builder.Reset();

    RequestReader reader;
    HLDPPacketType requestType = ReceiveRequest(reader);
    switch (requestType) {
    case HLDPPacketType::csBreakIn:
      // TODO: resend backtrace.
      continue; // The target is already stopped.
    case HLDPPacketType::csContinue:
      m_EndOfStepScopeID = kNoScope;
      SendReply(HLDPPacketType::scTargetRunning, builder);
      return;
    case HLDPPacketType::csStepIn:
      m_BreakInPending = true;
      SendReply(HLDPPacketType::scTargetRunning, builder);
      return;
    case HLDPPacketType::csStepOut:
      if (m_CallStack.size() >= 3)
        m_EndOfStepScopeID = m_CallStack[m_CallStack.size() - 3]->GetUniqueID();
      else if (m_CallStack.size() == 2)
        m_EndOfStepScopeID = kRootScope;
      SendReply(HLDPPacketType::scTargetRunning, builder);
      return;
    case HLDPPacketType::csStepOver:
      if (m_CallStack.size() >= 2)
        m_EndOfStepScopeID = m_CallStack[m_CallStack.size() - 2]->GetUniqueID();
      else
        m_EndOfStepScopeID = kRootScope;
      SendReply(HLDPPacketType::scTargetRunning, builder);
      return;
    case HLDPPacketType::csSetNextStatement:
      if (!pSkipThisInstruction) {
        SendErrorPacket("Cannot set next statement in this context");
        continue;
      } else if (!reader.ReadString(&expression) || !reader.ReadInt32(&ID)) {
        SendErrorPacket("Invalid set-next-statement request");
        continue;
      } else if (m_CallStack.size() == 0) {
        SendErrorPacket("Unknown CMake call stack");
        continue;
      } else {
        std::string canonicalRequestedPath =
            cmsys::SystemTools::GetRealPath(expression);
        std::string canonicalCurrentPath = cmsys::SystemTools::GetRealPath(
            m_CallStack[m_CallStack.size() - 1]->SourceFile);
        if (cmsysString_strcasecmp(canonicalCurrentPath.c_str(),
                                   canonicalRequestedPath.c_str()) != 0)
          SendErrorPacket("Cannot step to a different source file");
        else {
          m_NextOneBasedLineToExecute = ID;
          m_BreakInPending = true;
          *pSkipThisInstruction = true;
          SendReply(HLDPPacketType::scTargetRunning, builder);
          return;
        }
      }
    case HLDPPacketType::csDetach:
      m_Detached = true;
      SendReply(HLDPPacketType::scTargetRunning, builder);
      return;
    case HLDPPacketType::csTerminate:
      exit(0);
      cmSystemTools::SetFatalErrorOccured();
      return;
    case HLDPPacketType::csGetAllVariables: {
      if (!reader.ReadInt32(&ID)) {
        SendErrorPacket("Invalid frame for dump all variables");
        continue;
      }
      if (ID < 0 || ID >= m_CallStack.size())
        SendErrorPacket("Invalid frame ID");
      else {
        auto scope = m_CallStack[ID];
        auto defs = scope->Makefile->GetDefinitionsAndValues();

        std::cout << "Got here" << std::endl;
        builder.AppendInt32(defs.size());
        for (int i = 0; i < defs.size(); ++i) {
          auto [key, value] = defs[i];
          builder.AppendInt32(i);
          builder.AppendString(key);
          builder.AppendString("(CMake Expression)");
          builder.AppendString(value);
          builder.AppendInt32(0);
          builder.AppendInt32(0);
        }

        SendReply(HLDPPacketType::scAllVariables, builder);
      }
      continue;
    }
    case HLDPPacketType::csCreateExpression: {
      if (!reader.ReadInt32(&ID) || !reader.ReadString(&expression)) {
        SendErrorPacket("Invalid expression request");
        continue;
      }

      std::unique_ptr<ExpressionBase> pExpression;
      if (ID < 0 || ID >= m_CallStack.size())
        SendErrorPacket("Invalid frame ID");
      else {
        pExpression = CreateExpression(expression, *m_CallStack[ID]);
        if (pExpression) {
          pExpression->AssignedID = m_NextExpressionID++;

          builder.AppendInt32(pExpression->AssignedID);
          builder.AppendString(pExpression->Name);
          builder.AppendString(pExpression->Type);
          builder.AppendString(pExpression->Value);
          builder.AppendInt32(0);
          builder.AppendInt32(
              pExpression->ChildCountOrMinusOneIfNotYetComputed);

          m_ExpressionCache[pExpression->AssignedID] = std::move(pExpression);

          SendReply(HLDPPacketType::scExpressionCreated, builder);
        } else {
          SendErrorPacket("Failed to create expression");
        }
      }
      continue;
    }
    case HLDPPacketType::csQueryExpressionChildren:
      if (!reader.ReadInt32(&ID)) {
        SendErrorPacket("Invalid expression request");
        continue;
      } else {
        auto it = m_ExpressionCache.find(ID);
        if (it == m_ExpressionCache.end()) {
          SendErrorPacket("Invalid expression ID");
          continue;
        } else {
          if (!it->second->ChildrenRegistered) {
            it->second->ChildrenRegistered = true;
            for (auto &pChild : it->second->CreateChildren()) {
              pChild->AssignedID = m_NextExpressionID++;
              it->second->RegisteredChildren.push_back(pChild->AssignedID);
              m_ExpressionCache[pChild->AssignedID] = std::move(pChild);
            }
          }

          auto childCount = builder.AppendDelayedInt32();
          for (auto &id : it->second->RegisteredChildren) {
            auto it = m_ExpressionCache.find(id);
            if (it == m_ExpressionCache.end())
              continue;

            builder.AppendInt32(id);
            builder.AppendString(it->second->Name);
            builder.AppendString(it->second->Type);
            builder.AppendString(it->second->Value);
            builder.AppendInt32(0);
            builder.AppendInt32(
                it->second->ChildCountOrMinusOneIfNotYetComputed);

            (*childCount)++;
          }

          SendReply(HLDPPacketType::scExpressionChildrenQueried, builder);
        }
      }
      break;
    case HLDPPacketType::csSetExpressionValue:
      if (!reader.ReadInt32(&ID) || !reader.ReadString(&expression)) {
        SendErrorPacket("Invalid expression request");
        continue;
      } else {
        auto it = m_ExpressionCache.find(ID);
        if (it == m_ExpressionCache.end()) {
          SendErrorPacket("Invalid expression ID");
          continue;
        }
        std::string error;
        if (it->second->UpdateValue(expression, error))
          SendReply(HLDPPacketType::scExpressionUpdated, builder);
        else
          SendErrorPacket(error);
      }
      break;
    default:
      if (requestType > HLDPPacketType::BeforeFirstBreakpointRelatedCommand &&
          requestType < HLDPPacketType::AfterLastBreakpointRelatedCommand) {
        HandleBreakpointRelatedCommand(requestType, reader);
      } else
        SendErrorPacket(
            "Unexpected packet received while the target is stopped");
      break;
    }
  }

  m_ExpressionCache.clear();
}

std::unique_ptr<ExpressionBase>
HLDPServer::CreateExpression(const std::string &text, const RAIIScope &scope) {
  if (text == "ENV" || text == "$ENV")
    return std::make_unique<EnvironmentMetaExpression>();
  if (cmHasLiteralPrefix(text, "ENV{") && text.size() > 5) {
    std::string varName = text.substr(4, text.size() - 5);
    std::string value;
    if (cmSystemTools::GetEnv(varName, value)) {
      return std::make_unique<EnvironmentVariableExpression>(varName, value);
    } else
      return nullptr;
  }

  auto pValue =
      cmDefinitions::Get(text, scope.Position->Vars, scope.Position->Root);
  if (pValue)
    return std::make_unique<VariableExpression>(scope, text, pValue);

  auto pCache =
      scope.Makefile->GetCMakeInstance()->GetState()->GetCacheEntryValue(text);
  if (pCache) {
    std::string pCacheString = *pCache;
    return std::make_unique<VariableExpression>(scope, text, &pCacheString);
  }

  cmTarget *pTarget = scope.Makefile->FindTargetToUse(text, false);
  if (pTarget)
    return std::make_unique<TargetExpression>(pTarget);

  return nullptr;
}

} // namespace sp
