#pragma once
#include <Args.h>

#include <unistd.h>

#include <memory>

#include <HLDPServer/RequestReader.h>

class CMakeInstance;
using CMakeInstanceSP = std::shared_ptr<CMakeInstance>;

class CMakeInstance : public std::enable_shared_from_this<CMakeInstance> {
public:
  void Launch(Args &args);
  void RunCMake(Args &args);
  void TargetDidStop(RequestReader &reader);

private:
  pid_t m_pid;
};
