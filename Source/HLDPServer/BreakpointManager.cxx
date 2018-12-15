#include "cmsys/SystemTools.hxx"

#include "BreakpointManager.h"

#include "cmSystemTools.h"

sp::BreakpointManager::BreakpointObject *
sp::BreakpointManager::TryGetBreakpointAtLocation(const std::string &file,
                                                       int oneBasedLine) {
  auto it =
      m_BreakpointsByLocation.find(MakeCanonicalLocation(file, oneBasedLine));

  if (it == m_BreakpointsByLocation.end() || it->second.empty())
    return nullptr;

  return TryLookupBreakpointObject(*it->second.begin());
}

sp::BreakpointManager::BreakpointObject *
sp::BreakpointManager::TryGetBreakpointForFunction(
    const std::string &function) {
  auto it = m_BreakpointsByFunctionName.find(function);

  if (it == m_BreakpointsByFunctionName.end() || it->second.empty())
    return nullptr;

  return TryLookupBreakpointObject(*it->second.begin());
}

sp::BreakpointManager::UniqueBreakpointID
sp::BreakpointManager::CreateBreakpoint(const std::string &file,
                                             int oneBasedLine) {
  auto id = m_NextID++;
  auto location = MakeCanonicalLocation(file, oneBasedLine);
  if (location.Path.empty())
    return InvalidBreakpointID;
  m_BreakpointsByLocation[location].insert(id);
  m_BreakpointsByID[id] = std::make_unique<BreakpointObject>(id, location);
  return id;
}

sp::BreakpointManager::UniqueBreakpointID
sp::BreakpointManager::CreateBreakpoint(const std::string &function) {
  auto id = m_NextID++;
  CaseInsensitiveObjectName name = function;
  m_BreakpointsByFunctionName[name].insert(id);
  m_BreakpointsByID[id] = std::make_unique<BreakpointObject>(id, name);
  return id;
}

sp::BreakpointManager::UniqueBreakpointID
sp::BreakpointManager::CreateDomainSpecificBreakpoint(
    std::unique_ptr<DomainSpecificBreakpointExtension> &&extension) {
  auto id = m_NextID++;
  m_BreakpointsByID[id] =
      std::make_unique<BreakpointObject>(id, std::move(extension));
  return id;
}

void sp::BreakpointManager::DeleteBreakpoint(UniqueBreakpointID id) {
  auto it = m_BreakpointsByID.find(id);
  if (it == m_BreakpointsByID.end())
    return;

  // We may want to clear the remove the location record in case it was the
  // last breakpoint, but it should not cause any noticeable delays.
  m_BreakpointsByLocation[it->second->Location].erase(id);
  m_BreakpointsByFunctionName[it->second->FunctionName].erase(id);
  m_BreakpointsByID.erase(it);
}

sp::BreakpointManager::BreakpointObject *
sp::BreakpointManager::TryLookupBreakpointObject(UniqueBreakpointID id) {
  auto it = m_BreakpointsByID.find(id);
  if (it == m_BreakpointsByID.end())
    return nullptr;
  return it->second.get();
}

sp::BreakpointManager::CanonicalFileLocation
sp::BreakpointManager::MakeCanonicalLocation(const std::string &file,
                                                  int oneBasedLine) {
  auto it = m_CanonicalPathMap.find(file);
  if (it != m_CanonicalPathMap.end())
    return CanonicalFileLocation(it->second, oneBasedLine);

  std::string canonicalPath = cmsys::SystemTools::GetRealPath(file);
  m_CanonicalPathMap[file] = canonicalPath;
  return CanonicalFileLocation(canonicalPath, oneBasedLine);
}
