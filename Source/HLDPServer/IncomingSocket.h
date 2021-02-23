#pragma once

#include "cmSystemTools.h"

#ifdef _WIN32
#include <WinSock2.h>
typedef int socklen_t;
#else
#include <errno.h>
#include <netinet/ip.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int SOCKET;
static void closesocket(SOCKET socket) { close(socket); }
#endif

#include <glog/logging.h>

/*
        This is a very basic abstraction of the UNIX socket with blocking
   all-or-nothing I/O. Long-term, we may want to switch it to using libuv as
   the rest of CMake does, but the current basic implementation should be good
   enough for both Windows and Linux.
*/
class IncomingSocket {
private:
  SOCKET m_Socket, m_AcceptedSocket;

public:
  IncomingSocket(int tcpPort) : m_Socket(0), m_AcceptedSocket(0) {
    sockaddr_in addr = {
        AF_INET,
    };
    addr.sin_port = htons(tcpPort);

    LOG(INFO) << "socket";
    m_Socket = socket(AF_INET, SOCK_STREAM, 0);
    int enabled = 1;
    LOG(INFO) << "setsockopt";
    if (::setsockopt(m_Socket, SOL_SOCKET, SO_REUSEADDR,
                     reinterpret_cast<char *>(&enabled),
                     sizeof(enabled)) == -1) {
      cmSystemTools::Error("Failed to set SO_RESUSEADDR");
      cmSystemTools::Error(strerror(errno));
      return;
    }

    if (m_Socket < 0) {
      cmSystemTools::Error(
          "Failed to create a listening socket for the debug server.");
      cmSystemTools::Error(strerror(errno));
      return;
    }

    LOG(INFO) << "bind";
    if (bind(m_Socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
      closesocket(m_Socket);
      m_Socket = -1;
      cmSystemTools::Error(
          "Failed to bind the listening socket for the debug server.");
      cmSystemTools::Error(strerror(errno));
      return;
    }

    LOG(INFO) << "listen";
    if (listen(m_Socket, 1) < 0) {
      closesocket(m_Socket);
      m_Socket = -1;
      cmSystemTools::Error(
          "Failed to start the listening socket for the debug server.");
      cmSystemTools::Error(strerror(errno));
      return;
    }
  }

  bool Accept() {
    if (m_Socket < 0)
      return false;
    sockaddr_in addr;
    socklen_t len = sizeof(addr);
    LOG(INFO) << "accept";
    m_AcceptedSocket = accept(m_Socket, (struct sockaddr *)&addr, &len);
    if (m_AcceptedSocket < 0) {
      cmSystemTools::Error("Failed to accept a debugging connection.");
      cmSystemTools::Error(strerror(errno));
      return false;
    }

    return true;
  }

  bool HasIncomingData() {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(m_AcceptedSocket, &fds);

    timeval timeout = {
        0,
    };
    LOG(INFO) << "select";
    int result = select(m_AcceptedSocket + 1, &fds, NULL, NULL, &timeout);
    return result > 0;
  }

  bool Write(const void *pData, size_t size) {
    if (m_AcceptedSocket <= 0)
      return false;

    std::string res;

    for (int i = 0; i < size; ++i) {
      char c = ((char *)pData)[i];
      if (isalnum(c) || isprint(c))
        res += c;
      else
        res += "|" + std::to_string(c) + "|";
    }
    LOG(INFO) << "send: " + res;
    int done = send(m_AcceptedSocket, (const char *)pData, size, 0);
    return done == size;
  }

  bool ReadAll(void *pData, size_t size) {
    if (m_AcceptedSocket <= 0)
      return false;
    LOG(INFO) << "recv";
    int done = -1;
    int count = 0;
    while (done < 0 && count < 5) {
      done = recv(m_AcceptedSocket, (char *)pData, size, MSG_WAITALL);
      ++count;
    }
    if (done < 0) {
      cmSystemTools::Error("Recv error.");
    }
    return done == size;
  }

  ~IncomingSocket() {
    if (m_AcceptedSocket > 0)
      closesocket(m_AcceptedSocket);
    if (m_Socket > 0)
      closesocket(m_Socket);
  }
};
