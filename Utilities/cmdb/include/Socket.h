#pragma once

#include <unistd.h>
#include <iostream>

#ifdef _WIN32
#include <WinSock2.h>
typedef int socklen_t;
#else
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <sys/socket.h>
typedef int SOCKET;
static void closesocket(SOCKET socket) { close(socket); }
#endif

#include <HLDPServer/HLDP.h>
#include <HLDPServer/RequestReader.h>

/*
   This is a very basic abstraction of the UNIX socket with blocking
   all-or-nothing I/O. Long-term, we may want to switch it to using libuv as
   the rest of CMake does, but the current basic implementation should be good
   enough for both Windows and Linux.
*/
class Socket {

public:
  Socket() :m_Socket(-1) {}
  Socket(int tcpPort) : m_Socket(0) {

    sleep(1);
    if ((m_Socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      std::cout << "Failed to create a listening socket for the debug server.\n";
      return;
    }

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(tcpPort);

    if (inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) <= 0) {
      std::cout << "Invalid address\n";
    }

    if (connect(m_Socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
      printf("connect error: %s\n", strerror(errno));
      closesocket(m_Socket);
      m_Socket = -1;
      std::cout << "Failed to connect with ConnectSocket.\n";
    }

    char* buffer = new char[1024];
    int i = ReadAllNoWait(buffer, 1024);
    if (i == 0) {
      std::cerr << "cmdb <-> cmake socket closed unexpectedly.\n";
      exit(1);
    } else if (i == -1) {
      std::cerr << "cmdb <-> cmake socket error: " << strerror(errno) << '\n';
      exit(1);
    } else {
      char const* banner = "HLDPServer High-Level Debug Protocol";
      assert(strcmp(buffer, banner) == 0);
    }
  }

  bool HasIncomingData() {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(m_Socket, &fds);

    timeval timeout = {
        0,
    };
    int result = select(m_Socket + 1, &fds, NULL, NULL, &timeout);
    return result > 0;
  }

  bool Write(const void *pData, size_t size) {
    if (m_Socket <= 0)
      return false;
    int done = send(m_Socket, (const char *)pData, size, 0);
    return done == size;
  }

  bool ReadAll(void *pData, size_t size) {
    if (m_Socket <= 0)
      return false;
    int done = recv(m_Socket, (char *)pData, size, MSG_WAITALL);
    if (done < 0) {
      printf("ReadAll::recv %s\n", strerror(errno));
    } else {
      printf("%d bytes received: %ld\n", done, (long)pData);
    }
    return done == size;
  }

  int ReadAllNoWait(void *pData, size_t size) {
    if (m_Socket <= 0)
      return false;
    int done = recv(m_Socket, (char *)pData, size, 0);
    return done;
  }

  ~Socket() {
    if (m_Socket > 0)
      closesocket(m_Socket);
    if (m_Socket > 0)
      closesocket(m_Socket);
  }

protected:
  SOCKET m_Socket;
};

using SocketUP = std::unique_ptr<Socket>;
