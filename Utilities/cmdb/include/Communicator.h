#pragma once

#include <ForwardDeclare.h>

#include <HLDPServer/HLDP.h>
#include <HLDPServer/ReplyBuilder.h>
#include <HLDPServer/RequestReader.h>

#include <CMakeInstance.h>
#include <Socket.h>

#include <array>

class Communicator : public std::enable_shared_from_this<Communicator> {
public:
  Communicator() : m_listen_socket_up(nullptr), m_accepted_socket_up(nullptr) {}

  sp::HLDPPacketType ReceivePacket(RequestReader &reader);
  bool SendPacket(sp::HLDPPacketType packet_type, ReplyBuilder const *builder);

  void DestroySocket() {
    if (m_listen_socket_up)
      m_listen_socket_up.release();
    if (m_accepted_socket_up)
      m_accepted_socket_up.release();
  }

protected:
  std::array<char, 1024> buffer;

  SocketUP m_listen_socket_up;
  SocketUP m_accepted_socket_up;
};

