
#include <HLDPServer/HLDP.h>
#include <HLDPServer/ReplyBuilder.h>
#include <HLDPServer/RequestReader.h>
#include <IDECommunicator.h>
#include <Socket.h>

#ifndef WIN32
#include <sys/socket.h>
#endif

bool IDECommunicator::ConnectToIDE(int port_number) {
  m_listen_socket_up = std::make_unique<Socket>();
  m_listen_socket_up->Listen(port_number);
  m_accepted_socket_up = m_listen_socket_up->Accept();

  m_accepted_socket_up->Write("muffin", 6);

  ReplyBuilder b;
  b.AppendString("1");
  SendPacket(sp::HLDPPacketType::scHandshake, &b);

  RequestReader r;
  if (ReceivePacket(r) != sp::HLDPPacketType::csHandshake) {
    std::cout << "Failed to complete IDE handshake\n";
    return false;
  }

  return true;
}
