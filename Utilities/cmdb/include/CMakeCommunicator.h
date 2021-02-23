#pragma once
#include <Communicator.h>

class CMakeCommunicator : public Communicator {
public:
  void ConnectToCMakeInstance(CMakeInstanceSP cmake_instance_sp);
private:
  CMakeInstanceSP m_cmake_instance_sp;
};
