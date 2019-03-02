#pragma once

#include <HLDPServer/HLDP.h>
#include <HLDPServer/ReplyBuilder.h>
#include <HLDPServer/RequestReader.h>

#include <CMakeInstance.h>
#include <Socket.h>

#include <array>

class Communicator : public std::enable_shared_from_this<Communicator> {
public:
  Communicator() : m_socket_up(nullptr) {}
  void ConnectToCMakeInstance(CMakeInstance &cmake_instance);
  std::array<char, 1024> buffer;
  sp::HLDPPacketType ReceivePacket(RequestReader &reader);
  bool SendPacket(sp::HLDPPacketType packet_type, ReplyBuilder const *builder);

private:
  CMakeInstanceSP m_cmake_instance_sp;
  SocketUP m_socket_up;
};

using CommunicatorSP = std::shared_ptr<Communicator>;
