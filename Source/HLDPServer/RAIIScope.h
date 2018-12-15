#pragma once

using UniqueScopeID = int;
using UniqueExpressionID = int;

class cmCommand;
class cmMakefile;
class cmListFileFunction;

#include "cmStateTypes.h"

#include <string>

namespace sp {

class HLDPServer;

class RAIIScope {
private:
  sp::HLDPServer *m_pServer;
  UniqueScopeID m_UniqueID;

public:
  cmCommand *Command;
  cmMakefile *Makefile;
  const cmListFileFunction &Function;
  std::string SourceFile;
  cmStateDetail::PositionType Position;
  UniqueScopeID GetUniqueID() { return m_UniqueID; }

public:
  RAIIScope(sp::HLDPServer *pServer, cmCommand *pCommand, cmMakefile *pMakefile,
            const cmListFileFunction &function);

  RAIIScope(const RAIIScope &) = delete;
  void operator=(const RAIIScope &) = delete;

  ~RAIIScope();
};

} // end namespace sp
