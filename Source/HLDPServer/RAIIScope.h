#pragma once

using UniqueScopeID = int;
using UniqueExpressionID = int;

class cmMakefile;
struct cmListFileFunction;

#include "cmState.h"
#include "cmStateTypes.h"

#include <string>

namespace sp {

class HLDPServer;

class RAIIScope {
private:
  sp::HLDPServer *m_pServer;
  UniqueScopeID m_UniqueID;

public:
  cmState::Command Cmd;
  cmMakefile *Makefile;
  const cmListFileFunction &Function;
  std::string SourceFile;
  cmStateDetail::PositionType Position;
  UniqueScopeID GetUniqueID() { return m_UniqueID; }

public:
  RAIIScope(sp::HLDPServer *pServer, cmState::Command Cmd, cmMakefile *pMakefile,
            const cmListFileFunction &function);

  RAIIScope(const RAIIScope &) = delete;
  void operator=(const RAIIScope &) = delete;

  ~RAIIScope();
};

} // end namespace sp
