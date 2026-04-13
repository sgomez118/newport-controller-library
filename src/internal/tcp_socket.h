#ifndef NEWPORT_XPS_INTERNAL_TCP_SOCKET_H_
#define NEWPORT_XPS_INTERNAL_TCP_SOCKET_H_

// Platform socket includes — kept here in the internal header so they never
// leak into the public API surface.
#ifdef _WIN32
#  include <winsock2.h>
#  include <ws2tcpip.h>
using NativeSocket = SOCKET;
constexpr NativeSocket kInvalidNativeSocket = INVALID_SOCKET;
#else
#  include <fcntl.h>
#  include <netdb.h>
#  include <netinet/in.h>
#  include <sys/socket.h>
#  include <unistd.h>
using NativeSocket = int;
constexpr NativeSocket kInvalidNativeSocket = -1;
#endif

#include <chrono>
#include <expected>
#include <string>
#include <string_view>

#include "newport/xps/xps_error.h"

namespace newport::xps::internal {

// Synchronous TCP socket with timeout support.
//
// Provides the raw send/receive plumbing used by XpsController.  All
// operations are blocking and execute on the calling thread — one socket,
// one thread.
//
// The read buffer accumulates leftover bytes across ReadUntil calls so that
// pipelined or split XPS responses are reassembled correctly.
class TcpSocket {
 public:
  TcpSocket();
  ~TcpSocket();

  TcpSocket(const TcpSocket&)            = delete;
  TcpSocket& operator=(const TcpSocket&) = delete;
  TcpSocket(TcpSocket&&) noexcept;
  TcpSocket& operator=(TcpSocket&&) noexcept;

  // Establishes a TCP connection to `host`:`port`.
  // Resolves the host, creates the socket, and performs a non-blocking
  // connect with the given timeout.
  [[nodiscard]] std::expected<void, XpsError> Connect(
      std::string_view host, uint16_t port,
      std::chrono::milliseconds timeout);

  // Sends all bytes in `data`.  Loops internally to handle partial sends.
  [[nodiscard]] std::expected<void, XpsError> Send(std::string_view data);

  // Reads from the socket until `sentinel` appears in the accumulated data.
  // Returns everything up to and including the sentinel.  Any bytes received
  // beyond the sentinel are kept in the internal buffer for the next call.
  // Blocks for at most `timeout` total across all recv() calls.
  [[nodiscard]] std::expected<std::string, XpsError> ReadUntil(
      std::string_view sentinel, std::chrono::milliseconds timeout);

  // Shuts down and closes the socket.  Safe to call on an already-closed
  // socket.  Clears the read buffer.
  void Close();

  [[nodiscard]] bool IsConnected() const;

 private:
  NativeSocket socket_    = kInvalidNativeSocket;
  bool         connected_ = false;
  std::string  read_buffer_;  // bytes received but not yet returned to caller

#ifdef _WIN32
  bool wsa_initialized_ = false;
#endif
};

}  // namespace newport::xps::internal

#endif  // NEWPORT_XPS_INTERNAL_TCP_SOCKET_H_
