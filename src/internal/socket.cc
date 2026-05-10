#include "socket.h"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <mutex>
#include <span>
#include <string>
#include <string_view>
#include <utility>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <WS2tcpip.h>
#include <WinSock2.h>
#pragma comment(lib, "Ws2_32.lib")
#endif

namespace newport::xps::internal {

namespace {

// ---------------------------------------------------------------------------
// Platform glue
// ---------------------------------------------------------------------------

#ifdef _WIN32
using NativeHandle = SOCKET;
using SocklenCompat = int;
constexpr NativeHandle kPlatformInvalidSocket = INVALID_SOCKET;
constexpr int kPlatformSocketError = SOCKET_ERROR;
#endif

constexpr NativeHandle ToNative(std::uintptr_t h) noexcept {
  return static_cast<NativeHandle>(h);
}

constexpr std::uintptr_t FromNative(NativeHandle s) noexcept {
  return static_cast<std::uintptr_t>(s);
}

#ifdef _WIN32
void EnsureWinsockStarted() {
  static std::once_flag once;
  std::call_once(once, [] {
    WSADATA wsa{};
    (void)WSAStartup(MAKEWORD(2, 2), &wsa);
  });
}

int LastSocketError() noexcept { return WSAGetLastError(); }

std::string FormatErrno(int err) {
  char* buf = nullptr;
  DWORD len = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                 FORMAT_MESSAGE_FROM_SYSTEM |
                                 FORMAT_MESSAGE_IGNORE_INSERTS,
                             nullptr, static_cast<DWORD>(err),
                             MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                             reinterpret_cast<LPSTR>(&buf), 0, nullptr);
  std::string msg;

  if (len && buf) {
    msg.assign(buf, len);
    // Trim trailing CRLF that FOrmatMessageA likes to append.
    while (!msg.empty() && (msg.back() == '\r' || msg.back() == '\n' ||
                            msg.back() == ' ' || msg.back() == '.')) {
      msg.pop_back();
    }
    LocalFree(buf);
  } else {
    msg = "winsock error " + std::to_string(err);
  }
  return msg;
}

void CloseHandle(NativeHandle s) noexcept {
  if (s != kPlatformInvalidSocket) {
    ::shutdown(s, SD_BOTH);
    ::closesocket(s);
  }
}
#endif

[[nodiscard]] Error MakeError(int code, std::string_view what, int sys_err) {
  std::string msg{what};
  msg += ": ";
  msg += FormatErrno(sys_err);
  msg += " (errno=)";
  msg += std::to_string(sys_err);
  msg += ')';
  return Error{code, std::move(msg)};
}

[[nodiscard]] Error MakeError(int code, std::string what) {
  return Error{code, std::move(what)};
}

// ---------------------------------------------------------------------------
// Non-blocking connect with timeout
// ---------------------------------------------------------------------------

[[nodiscard]] bool SetNonBlocking(NativeHandle s, bool nb) noexcept {
#ifdef _WIN32
  u_long mode = nb ? 1u : 0u;
  return ::ioctlsocket(s, FIONBIO, &mode) == 0;
#endif
}

[[nodiscard]] int WaitForConnect(NativeHandle s,
                                 std::chrono::milliseconds timeout) noexcept {
#ifdef _WIN32
  fd_set wset, eset;
  FD_ZERO(&wset);
  FD_ZERO(&eset);
  FD_SET(s, &wset);
  FD_SET(s, &eset);
  timeval tv{};
  tv.tv_sec = static_cast<long>(timeout.count() / 1000);
  tv.tv_usec = static_cast<long>((timeout.count() % 1000) * 1000);
  int n = ::select(0, nullptr, &wset, &eset, &tv);
  if (n == 0) return 1;  // timeout
  if (n == kPlatformSocketError) return -1;
  if (FD_ISSET(s, &eset)) return -1;
  // Check SO_ERROR -- connect can complete with an asynchronous failure.
  int so_err = 0;
  SocklenCompat len = sizeof(so_err);
  if (::getsockopt(s, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&so_err),
                   &len) != 0 ||
      so_err != 0) {
    WSASetLastError(so_err);
    return -1;
  }
  return 0;
#endif
}

[[nodiscard]] Result<void> SetTimeoutImpl(NativeHandle s, int optname,
                                          std::chrono::milliseconds timeout,
                                          const char* what) {
  if (timeout < std::chrono::milliseconds{1}) {
    timeout = std::chrono::milliseconds{1};
  }

#ifdef _WIN32
  DWORD ms = static_cast<DWORD>(timeout.count());
  int rc = ::setsockopt(s, SOL_SOCKET, optname,
                        reinterpret_cast<const char*>(&ms), sizeof(ms));
#endif

  if (rc == kPlatformSocketError) {
    return std::unexpected(MakeError(transport_error::kSocketSetoptFailed, what,
                                     LastSocketError()));
  }
  return {};
}

}  // namespace

// ---------------------------------------------------------------------------
// Socket
// ---------------------------------------------------------------------------

Socket::Socket(Socket&& other) noexcept : handle_(other.handle_) {
  other.handle_ = kInvalidHandle;
}

Socket& Socket::operator=(Socket&& other) noexcept {
  if (this != &other) {
    Close();
    handle_ = other.handle_;
    other.handle_ = kInvalidHandle;
  }
  return *this;
}

Socket::~Socket() { Close(); }

void Socket::Close() noexcept {
  if (handle_ != kInvalidHandle) {
    CloseHandle(ToNative(handle_));
    handle_ = kInvalidHandle;
  }
}

Result<Socket> Socket::Connect(std::string_view host, int port,
                               std::chrono::milliseconds connect_timeout) {
  EnsureWinsockStarted();

  if (host.empty()) {
    return std::unexpected(
        MakeError(transport_error::kSocketResolveFailed, "host is empty"));
  }

  if (port <= 0 || port > 65535) {
    return std::unexpected(
        MakeError(transport_error::kSocketConnectFailed, "port out of range"));
  }

  // getaddrinfo wants a NUL-terminated host string.
  std::string host_z{host};
  std::string port_z = std::to_string(port);

  addrinfo hints{};
  hints.ai_family = AF_UNSPEC;  // IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  addrinfo* res = nullptr;
  int gai = ::getaddrinfo(host_z.c_str(), port_z.c_str(), &hints, &res);
  if (gai != 0 || res == nullptr) {
#ifdef _WIN32
    return std::unexpected(MakeError(transport_error::kSocketResolveFailed,
                                     "getaddrinfo", WSAGetLastError()));
#endif
  }

  // Try each resolved address in order; keep the last error to report.
  Error last_err{transport_error::kSocketConnectFailed, "no addresses tried"};
  for (addrinfo* p = res; p != nullptr; p = p->ai_next) {
    NativeHandle s = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);

    if (s == kPlatformInvalidSocket) {
      last_err = MakeError(transport_error::kSocketCreateFailed, "socket()",
                           LastSocketError());
      continue;
    }

    if (!SetNonBlocking(s, true)) {
      last_err = MakeError(transport_error::kSocketSetoptFailed,
                           "set non-blocking", LastSocketError());
      CloseHandle(s);
      continue;
    }

    int cr =
        ::connect(s, p->ai_addr, static_cast<SocklenCompat>(p->ai_addrlen));
    bool in_progress = false;
    if (cr == kPlatformSocketError) {
      int err = LastSocketError();
#ifdef _WIN32
      in_progress = (err == WSAEWOULDBLOCK || err == WSAEINPROGRESS);
#endif
      if (!in_progress) {
        last_err =
            MakeError(transport_error::kSocketConnectFailed, "connect()", err);
        CloseHandle(s);
        continue;
      }
    }

    if (in_progress) {
      int wr = WaitForConnect(s, connect_timeout);
      if (wr > 0) {
        last_err = Error{transport_error::kSocketTimeout, "connect timed out"};
        CloseHandle(s);
        continue;
      }
      if (wr < 0) {
        last_err = MakeError(transport_error::kSocketConnectFailed,
                             "connect (async)", LastSocketError());
        CloseHandle(s);
        continue;
      }
    }

    // Back to blocking mode for norma send/recv
    if (!SetNonBlocking(s, false)) {
      last_err = MakeError(transport_error::kSocketSetoptFailed, "set blocking",
                           LastSocketError());
      CloseHandle(s);
      continue;
    }

    int one = 1;
    (void)::setsockopt(s, IPPROTO_TCP, TCP_NODELAY,
                       reinterpret_cast<const char*>(&one), sizeof(one));

    ::freeaddrinfo(res);

    Socket sock(FromNative(s));

    // Default timeouts to 1s, matching Newport's OpenInstrument helper
    if (auto r = sock.SetSendTimeout(std::chrono::seconds{1}); !r) {
      return std::unexpected(std::move(r.error()));
    }
    if (auto r = sock.SetRecvTimeout(std::chrono::seconds{1}); !r) {
      return std::unexpected(std::move(r.error()));
    }

    return sock;
  }

  ::freeaddrinfo(res);
  return std::unexpected(std::move(last_err));
}

Result<void> Socket::SendAll(std::span<const std::byte> data) {
  if (!IsOpen()) {
    return std::unexpected(
        MakeError(transport_error::kSocketSendFailed, "socket not open"));
  }

  const std::byte* p = data.data();
  std::size_t remaining = data.size();
  NativeHandle s = ToNative(handle_);

  while (remaining > 0) {
    std::size_t chunk = std::min<std::size_t>(remaining, 1u << 20);

#ifdef _WIN32
    int n =
        ::send(s, reinterpret_cast<const char*>(p), static_cast<int>(chunk), 0);
#endif

    if (n == kPlatformSocketError) {
      int err = LastSocketError();
#ifdef _WIN32
      if (err == WSAETIMEDOUT) {
        return std::unexpected(
            Error{transport_error::kSocketTimeout, "send timed out"});
      }
#endif
      return std::unexpected(
          MakeError(transport_error::kSocketSendFailed, "send()", err));
    }

    if (n == 0) {
      return std::unexpected(Error{transport_error::kSocketClosedByPeer,
                                   "peer closed connection during send"});
    }

    p += static_cast<std::size_t>(n);
    remaining -= static_cast<std::size_t>(n);
  }

  return {};
}

Result<std::size_t> Socket::RecvSome(std::span<std::byte> out) {
  if (!IsOpen()) {
    return std::unexpected(
        MakeError(transport_error::kSocketRecvFailed, "socket not open"));
  }
  if (out.empty()) {
    return std::size_t{0};
  }

  NativeHandle s = ToNative(handle_);
  std::size_t cap = std::min<std::size_t>(out.size(), 1u << 20);

#ifdef _WIN32
  int n =
      ::recv(s, reinterpret_cast<char*>(out.data()), static_cast<int>(cap), 0);
#endif

  if (n == kPlatformSocketError) {
    int err = LastSocketError();
#ifdef _WIN32
    if (err == WSAETIMEDOUT) {
      return std::unexpected(
          Error{transport_error::kSocketTimeout, "recv timed out"});
    }
#endif
    return std::unexpected(
        MakeError(transport_error::kSocketRecvFailed, "recv()", err));
  }

  if (n == 0) {
    return std::unexpected(
        Error{transport_error::kSocketClosedByPeer, "peer closed connection"});
  }

  return static_cast<std::size_t>(n);
}

Result<void> Socket::SetSendTimeout(std::chrono::milliseconds timeout) {
  if (!IsOpen()) {
    return std::unexpected(
        MakeError(transport_error::kSocketSetoptFailed, "socket not open"));
  }
  return SetTimeoutImpl(ToNative(handle_), SO_SNDTIMEO, timeout, "SO_SNDTIMEO");
}

Result<void> Socket::SetRecvTimeout(std::chrono::milliseconds timeout) {
  if (!IsOpen()) {
    return std::unexpected(
        MakeError(transport_error::kSocketSetoptFailed, "socket not open"));
  }
  return SetTimeoutImpl(ToNative(handle_), SO_RCVTIMEO, timeout, "SO_RCVTIMEO");
}

}  // namespace newport::xps::internal
