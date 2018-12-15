#pragma once

#include <HLDPServer/HLDP.h>
#include <HLDPServer/ReplyBuilder.h>
#include <HLDPServer/RequestReader.h>

#include <CMakeInstance.h>
#include <Socket.h>


class Communicator {
public:
  Communicator() :m_socket_up(nullptr) { }
  void ConnectToCMakeInstance(CMakeInstance& cmake_instance);
  sp::HLDPPacketType ReceiveRequest();

private:
  CMakeInstanceSP m_cmake_instance_sp;
  SocketUP m_socket_up;
  ReplyBuilder m_reply_builder;
  RequestReader m_packet_reader;
};
