#include "mock_xps_server.h"

#include <gtest/gtest.h>

#ifdef _WIN32
#  include <winsock2.h>
#  include <ws2tcpip.h>
// Avoid the need for a separate Ws2_32 pragma — CMake links it for us.
using SockFd = SOCKET;
constexpr SockFd kInvalidSock = INVALID_SOCKET;
#  define CLOSE_SOCK(fd) ::closesocket(fd)
#  define SEND_SIZE_T int
#else
#  include <arpa/inet.h>
#  include <netinet/in.h>
#  include <sys/socket.h>
#  include <unistd.h>
using SockFd = int;
constexpr SockFd kInvalidSock = -1;
#  define CLOSE_SOCK(fd) ::close(fd)
#  define SEND_SIZE_T std::size_t
#endif

namespace {

SockFd ToSockFd(intptr_t fd) { return static_cast<SockFd>(fd); }

}  // namespace

MockXpsServer::~MockXpsServer() { Stop(); }

void MockXpsServer::Expect(std::string command, std::string response) {
  expectations_.push_back({std::move(command), std::move(response)});
}

uint16_t MockXpsServer::Start() {
#ifdef _WIN32
  WSADATA wsa_data;
  WSAStartup(MAKEWORD(2, 2), &wsa_data);
#endif

  SockFd listen_sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  listen_fd_ = static_cast<intptr_t>(listen_sock);

#ifndef _WIN32
  int opt = 1;
  setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

  struct sockaddr_in addr = {};
  addr.sin_family      = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  addr.sin_port        = htons(0);  // ask the OS for a free port

  ::bind(listen_sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));

  // Retrieve the port the OS assigned.
  socklen_t addr_len = sizeof(addr);
  getsockname(listen_sock, reinterpret_cast<sockaddr*>(&addr), &addr_len);
  port_ = ntohs(addr.sin_port);

  ::listen(listen_sock, 1);

  running_ = true;
  thread_  = std::thread(&MockXpsServer::ServeLoop, this);

  return port_;
}

void MockXpsServer::Stop() {
  running_ = false;

  // Close the listen socket to unblock accept() in ServeLoop.
  if (listen_fd_ != -1) {
    CLOSE_SOCK(ToSockFd(listen_fd_));
    listen_fd_ = -1;
  }

  // Close the client socket to unblock any pending recv() in ServeLoop.
  if (client_fd_ != -1) {
    CLOSE_SOCK(ToSockFd(client_fd_));
    client_fd_ = -1;
  }

  if (thread_.joinable()) {
    thread_.join();
  }

#ifdef _WIN32
  WSACleanup();
#endif
}

void MockXpsServer::VerifyAllExpectationsMatched() {
  EXPECT_EQ(next_expectation_, expectations_.size())
      << next_expectation_ << " of " << expectations_.size()
      << " MockXpsServer expectations were consumed";
}

void MockXpsServer::ServeLoop() {
  SockFd listen_sock = ToSockFd(listen_fd_);

  // Use select() with a short poll interval so we can notice Stop() quickly
  // even if no client ever connects.
  while (running_) {
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(listen_sock, &read_fds);

    struct timeval tv = {};
    tv.tv_sec  = 0;
    tv.tv_usec = 50'000;  // 50 ms poll

    const int sel = ::select(static_cast<int>(listen_sock + 1),
                             &read_fds, nullptr, nullptr, &tv);
    if (sel <= 0) continue;  // timeout or error — loop and re-check running_

    SockFd client_sock = ::accept(listen_sock, nullptr, nullptr);
    if (client_sock == kInvalidSock) return;  // listen socket was closed

    client_fd_ = static_cast<intptr_t>(client_sock);
    break;
  }

  if (!running_) return;

  SockFd client_sock = ToSockFd(client_fd_);
  std::string recv_buf;
  char        buf[4096];

  while (running_ && next_expectation_ < expectations_.size()) {
    // Read bytes until we have a full command (terminated by '\n').
    while (recv_buf.find('\n') == std::string::npos) {
#ifdef _WIN32
      const int received = ::recv(client_sock, buf, sizeof(buf), 0);
      if (received <= 0) goto done;
#else
      const ssize_t received = ::recv(client_sock, buf, sizeof(buf), 0);
      if (received <= 0) goto done;
#endif
      recv_buf.append(buf, static_cast<size_t>(received));
    }

    {
      const auto newline = recv_buf.find('\n');
      const std::string command = recv_buf.substr(0, newline + 1);
      recv_buf.erase(0, newline + 1);

      const auto& expectation = expectations_[next_expectation_];
      EXPECT_EQ(command, expectation.command)
          << "MockXpsServer: unexpected command at expectation index "
          << next_expectation_;

      const std::string& response = expectation.response;
      ::send(client_sock, response.c_str(),
             static_cast<SEND_SIZE_T>(response.size()), 0);

      ++next_expectation_;
    }
  }

done:
  CLOSE_SOCK(client_sock);
  client_fd_ = -1;
}
