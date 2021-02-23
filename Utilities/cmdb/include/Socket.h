#pragma once

#include <iostream>

#ifdef _WIN32
#include <WinSock2.h>
typedef int socklen_t;
#else
#include <arpa/inet.h>
#include <assert.h>
#include <netinet/ip.h>
#include <glog/logging.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int SOCKET;
static void closeSocket(SOCKET socket) { close(socket); }
#endif

#include <memory>

#include <HLDPServer/HLDP.h>
#include <HLDPServer/RequestReader.h>

#include <glog/logging.h>

class Socket {

public:
  Socket() {};

  void Listen(int tcpPort) {
    LOG(INFO) << "CMDB::Socket::Listen -- socket()";
    if ((m_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      LOG(INFO) << "CMDB::Socket::Listen -- Failed to create socket";
      return;
    }

    int enabled = 1;
    LOG(INFO) << "CMDB::Socket::Listen -- setsockopt()";
    if (::setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR,
                     reinterpret_cast<char *>(&enabled),
                     sizeof(enabled)) == -1) {
      LOG(INFO) << "CMDB::Socket::Listen -- Failed to set SO_RESUSEADDR";
      std::cout << strerror(errno) << '\n';
      return;
    }
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(tcpPort);

    LOG(INFO) << "CMDB::Socket::Listen -- bind()";
    if (bind(m_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
      closeSocket(m_socket);
      m_socket = -1;
      std::cout << "Failed to bind the listening socket for the ide server.\n";
      return;
    }

    LOG(INFO) << "CMDB::Socket::Listen -- listen()";
    if (listen(m_socket, 1) < 0) {
      closeSocket(m_socket);
      m_socket = -1;
      std::cout << "Failed to start the listening socket for the ide server.\n";
      return;
    }
  }

  std::unique_ptr<Socket> Accept() {
    if (m_socket < 0)
      return nullptr;

    sockaddr_in addr;
    socklen_t len = sizeof(addr);

    LOG(INFO) << "CMDB::Socket::Accept -- accept()";
    int accepted_socket = accept(m_socket, (struct sockaddr *)&addr, &len);
    if (accepted_socket < 0) {
      std::cout << "Failed to accept a ide connection.\n";
      return nullptr;
    }

    auto s = new Socket;
    s->m_socket = accepted_socket;
    return std::unique_ptr<Socket>(s);
  }

  void Connect(int tcpPort) {
    LOG(INFO) << "Socket::Connect entered";
    if ((m_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      std::cout << "Failed to create socket.\n";
      std::cout << strerror(errno) << '\n';
      return;
   } else {
     LOG(INFO) << "Socket::Connect created socket: " << m_socket;
   }

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(tcpPort);

    LOG(INFO) << "Socket::Connect -- inet_pton()";
    if (inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) <= 0) {
      std::cerr << "Invalid address\n";
      exit(1);
    }

    LOG(INFO) << "CMDB::Socket::Connect -- connect()";
    if (connect(m_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
      LOG(INFO) << "Failed to connect - error: " << strerror(errno) << '\n';
      closeSocket(m_socket);
      m_socket = -1;
      return;
    }
    LOG(INFO) << "Socket::Connect succeeded";
  }

  bool HasIncomingData() {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(m_socket, &fds);

    timeval timeout;
    memset(&timeout, 0, sizeof(timeout));

    int result = select(m_socket + 1, &fds, nullptr, nullptr, &timeout);
    return result > 0;
  }

  bool Write(const void *pData, size_t size) {
    if (m_socket <= 0)
      return false;
    LOG(INFO) << "CMDB::Socket::Write -- send() -- " << pData;
    auto done = send(m_socket, (const char *)pData, size, 0);
    return static_cast<size_t>(done) == size;
  }

  bool ReadAll(void *pData, size_t size) {
    if (m_socket <= 0)
      return false;
    LOG(INFO) << "ReadAll -- recv";
    auto done = recv(m_socket, (char *)pData, size, MSG_WAITALL);
    if (done < 0) {
      std::cerr << "ReadAll::recv: " << strerror(errno) << '\n';
    }
    return static_cast<size_t>(done) == size;
  }

  int ReadAllNoWait(void *pData, size_t size) {
    if (m_socket <= 0)
      return false;
    int done = recv(m_socket, (char *)pData, size, MSG_WAITALL);
    return done;
  }

  bool IsValid() {
    return m_socket != -1;
  }

  ~Socket() {
    if (m_socket > 0)
      closeSocket(m_socket);
    if (m_socket > 0)
      closeSocket(m_socket);
  }

protected:
  SOCKET m_socket = -1;
};

using SocketUP = std::unique_ptr<Socket>;
