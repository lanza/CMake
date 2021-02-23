#include <CMakeCommunicator.h>
#include <Socket.h>
#include <glog/logging.h>

#include <HLDPServer/HLDP.h>

void ReadHLDPBanner(Socket &sock) {
  int len = sizeof(sp::HLDPBanner);
  char *buffer = new char[len];
  int i = sock.ReadAllNoWait(buffer, len);
  std::string res;

  if (i == 0) {
    std::cerr << "cmdb <-> cmake socket closed unexpectedly.\n";
    exit(1);
  } else if (i == -1) {
    std::cerr << "cmdb <-> cmake socket error: " << strerror(errno) << '\n';
    exit(1);
  } else {
    std::string read{buffer};
    std::string banner{sp::HLDPBanner};
    assert(read == banner);
  }

}

void CMakeCommunicator::ConnectToCMakeInstance(
    CMakeInstanceSP cmake_instance_sp) {
  m_cmake_instance_sp = cmake_instance_sp;

  m_accepted_socket_up = std::make_unique<Socket>();
  auto port_number = m_cmake_instance_sp->GetPortNumber();

  int try_count = 0;
  do {
    if (try_count > 0) {
      sleep(1);
    }
    m_accepted_socket_up->Connect(port_number);
    try_count++;
  } while (!m_accepted_socket_up->IsValid() && try_count < 5);

  LOG(INFO) << "Read HLDP starting";
  ReadHLDPBanner(*m_accepted_socket_up.get());
  LOG(INFO) << "Read HLDP banner";

  // handshake
  RequestReader reader;
  while (true) {
    LOG(INFO) << "Starting to look for handshake -- Before ReceivePacket";
    sp::HLDPPacketType packet = ReceivePacket(reader);
    LOG(INFO) << "Starting to look for handshake -- After ReceivePacket";
    if (packet == sp::HLDPPacketType::scHandshake) {
      LOG(INFO) << "SendPacket";
      SendPacket(sp::HLDPPacketType::csHandshake, nullptr);
      break;
    } else if (packet == sp::HLDPPacketType::scDebugMessage) {
      (void)0;
    } else {
      std::cerr << "Unexpected packet: " << (unsigned)packet << '\n';
    }
  }

  // target stopped
  while (true) {
    LOG(INFO) << "Starting to wait for target to stop";
    sp::HLDPPacketType packet = ReceivePacket(reader);
    if (packet == sp::HLDPPacketType::scTargetStopped) {
      m_cmake_instance_sp->TargetDidStop(reader);
      break;
    } else if (packet == sp::HLDPPacketType::scDebugMessage) {
      (void)0;
    } else {
      std::cerr << "Unexpected packet: " << (unsigned)packet << '\n';
      break;
    }
  }
}
