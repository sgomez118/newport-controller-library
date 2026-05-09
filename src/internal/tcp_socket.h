#ifndef NEWPORT_XPS_INTERNAL_TCP_SOCKET_H_
#define NEWPORT_XPS_INTERNAL_TCP_SOCKET_H_

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
using NativeSocket = SOCKET;
constexpr NativeSocket kInvalidNativeSocket = INVALID_SOCKET;
#else
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
using NativeSocket = int;
constexpr NativeSocket kInvalidNativeSocket = -1;
#endif

#include <chrono>
#include <expected>
#include <string>
#include <string_view>

namespace newport::xps::internal {

class TcpSocket {
 public:
  TcpSocket();
  ~TcpSocket();

  TcpSocket(const TcpSocket&) = delete;
  TcpSocket& operator=(const TcpSocket&) = delete;
  TcpSocket(TcpSocket&&) noexcept;
  TcpSocket& operator=(TcpSocket&&) noexcept;

  [[nodiscard]] int Connect(std::string_view host, uint16_t port,
                            std::chrono::milliseconds timeout);

  [[nodiscard]] int Send(std::string_view data);

  [[nodiscard]] int Receive(std::chrono::milliseconds timeout);

  void Close();

  [[nodiscard]] bool IsConnected() const;

 private:
  NativeSocket socket_ = kInvalidNativeSocket;
  bool connected_ = false;
  std::string read_buffer_;

#ifdef _WIN32
  bool wsa_initialized_ = false;
#endif
};

}  // namespace newport::xps::internal

#endif  // NEWPORT_XPS_INTERNAL_TCP_SOCKET_H_
