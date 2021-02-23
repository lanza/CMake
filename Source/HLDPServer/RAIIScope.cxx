
#include "RAIIScope.h"
#include "HLDPServer.h"

#include "cmMakefile.h"
namespace sp {
using namespace sp;

RAIIScope::RAIIScope(HLDPServer *pServer, cmState::Command Cmd,
                     cmMakefile *pMakefile, const cmListFileFunction &function)
    : m_pServer(pServer), Cmd(Cmd), Makefile(pMakefile),
      Function(function), m_UniqueID(pServer->m_NextScopeID++),
      Position(pMakefile->GetStateSnapshot().GetPositionForDebugging()) {
  pServer->m_CallStack.push_back(this);
  SourceFile = Makefile->GetStateSnapshot().GetExecutionListFile();
}

RAIIScope::~RAIIScope() {
  if (m_pServer->m_CallStack.back() != this) {
    cmSystemTools::Error("CMake scope imbalance detected");
    cmSystemTools::SetFatalErrorOccured();
  }

  if (m_UniqueID == m_pServer->m_EndOfStepScopeID)
    m_pServer->m_BreakInPending = true; // We are stepping out of a function
                                        // scope where we were supposed to stop.

  m_pServer->m_CallStack.pop_back();
}
} // namespace sp
