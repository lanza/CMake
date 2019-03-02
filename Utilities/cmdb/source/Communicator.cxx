#include <Communicator.h>
#include <HLDPServer/HLDP.h>
#include <HLDPServer/ReplyBuilder.h>
#include <HLDPServer/RequestReader.h>

#include <sys/socket.h>

extern int SOCK;

void Communicator::ConnectToCMakeInstance(CMakeInstance &cmake_instance) {
  m_cmake_instance_sp = cmake_instance.shared_from_this();

  m_socket_up = std::make_unique<Socket>(SOCK);

  sp::HLDPPacketType packet = ReceivePacket();
  if (packet == sp::HLDPPacketType::scHandshake) {
    SendPacket(sp::HLDPPacketType::csHandshake, nullptr);
  } else {
    std::cerr << "Unexpected packet: " << (unsigned)packet << '\n';
  }
}

sp::HLDPPacketType Communicator::ReceivePacket() {
  RequestReader reader;
  sp::HLDPPacketHeader header;

  if (!m_socket_up->ReadAll(&header, sizeof(header))) {
    std::cerr << "Error receiving cmake packet: " << strerror(errno)
              << std::endl;
    exit(99);
  }

  void *buffer_ptr = reader.Reset(header.PayloadSize);
  if (header.PayloadSize != 0) {
    if (!m_socket_up->ReadAll(buffer_ptr, header.PayloadSize)) {
      std::cerr << "Error receiving cmake packet: " << strerror(errno)
                << std::endl;
      exit(99);
    }
  }

  return (sp::HLDPPacketType)header.Type;
}

bool Communicator::SendPacket(sp::HLDPPacketType packet_type,
                              ReplyBuilder const *builder) {
  unsigned size = 0;
  if (builder != nullptr) {
    size = builder->GetBuffer().size();
  }
  sp::HLDPPacketHeader header = {(unsigned)packet_type, size};

  static int failCount = 0;

  if (!m_socket_up->Write(&header, sizeof(header))) {
    std::cerr << "Failed to write debug protocol header from cmdb.\n";
    exit(1);
  }
  failCount = 0;

  if (builder == nullptr || builder->GetBuffer().size() == 0) {
    // packet has no payload
    return true;
  }

  if (!m_socket_up->Write(builder->GetBuffer().data(), header.PayloadSize)) {
    std::cerr << "Failed to write debug protocol payload from cmdb.\n";
    exit(1);
  }

  return true;
}
