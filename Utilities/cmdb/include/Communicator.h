#pragma once

#include <HLDPServer/HLDP.h>
#include <HLDPServer/ReplyBuilder.h>
#include <HLDPServer/RequestReader.h>

#include <CMakeInstance.h>
#include <Socket.h>

class Communicator : public std::enable_shared_from_this<Communicator> {
public:
  Communicator() : m_socket_up(nullptr) {}
  void ConnectToCMakeInstance(CMakeInstance &cmake_instance);
  sp::HLDPPacketType ReceivePacket();
  bool SendPacket(sp::HLDPPacketType packet_type, ReplyBuilder const *builder);

private:
  CMakeInstanceSP m_cmake_instance_sp;
  SocketUP m_socket_up;
};

using CommunicatorSP = std::shared_ptr<Communicator>;
