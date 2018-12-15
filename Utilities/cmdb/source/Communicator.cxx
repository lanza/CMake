#include <Communicator.h>
#include <HLDPServer/HLDP.h>
#include <HLDPServer/RequestReader.h>
#include <HLDPServer/ReplyBuilder.h>


#include <sys/socket.h>

void Communicator::ConnectToCMakeInstance(CMakeInstance& cmake_instance) {
  m_cmake_instance_sp = cmake_instance.shared_from_this();

  m_socket_up = std::make_unique<Socket>(9286);

  sp::HLDPPacketType packet = ReceiveRequest();
  if (packet == sp::HLDPPacketType::scHandshake) {
    printf("Received scHandshake\n");

    sp::HLDPPacketHeader header = {(unsigned)sp::HLDPPacketType::csHandshake, 0};
    m_socket_up->Write(&header, sizeof(header));
    printf("sent csHandshake\n");
  } else {
    printf("%d\n", (unsigned)packet);
  }
}

sp::HLDPPacketType Communicator::ReceiveRequest() {
  RequestReader reader;
  sp::HLDPPacketHeader header;

  if (!m_socket_up->ReadAll(&header, sizeof(header))) {
    printf("idk! %s\n", strerror(errno));
    std::cout << "Error1?" << std::endl;
    exit(99);
  }

  void *buffer_ptr = reader.Reset(header.PayloadSize);
  if (header.PayloadSize != 0) {
    if (!m_socket_up->ReadAll(buffer_ptr, header.PayloadSize)) {
      printf("idk! %s\n", strerror(errno));
      std::cout << "Error2?" << std::endl;
      exit(99);
    }
  }

  return (sp::HLDPPacketType)header.Type;
}

// recv 1024 - banner
// packet - handshake
// 
