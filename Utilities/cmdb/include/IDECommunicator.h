#pragma once

#include <Communicator.h>

class IDECommunicator : public Communicator {
public:
  bool ConnectToIDE(int port_number);
};
