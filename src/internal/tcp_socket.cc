#include "internal/tcp_socket.h"

#include <cstring>
#include <string>

namespace newport::xps::internal {

namespace {

// Builds an XpsError from the most recent platform socket error.
// `context` is the operation name (e.g. "connect", "recv") that goes into
// XpsError::command so the caller can see exactly which call failed.
XpsError LastSocketError(std::string_view context) {
#ifdef _WIN32
  int code = WSAGetLastError();
  char msg[256] = {};
  FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                 nullptr, static_cast<DWORD>(code), 0, msg, sizeof(msg),
                 nullptr);
  // Strip trailing whitespace/newlines that FormatMessage appends.
  std::string message(msg);
  while (!message.empty() &&
         (message.back() == '\r' || message.back() == '\n' ||
          message.back() == ' ')) {
    message.pop_back();
  }
  return XpsError{code, std::string(context), message};
#else
  return XpsError{errno, std::string(context), std::strerror(errno)};
#endif
}

// Sets a send or receive timeout on an already-connected socket.
void SetSocketTimeout(NativeSocket sock, int option,
                      std::chrono::milliseconds timeout) {
#ifdef _WIN32
  DWORD ms = static_cast<DWORD>(timeout.count());
  setsockopt(sock, SOL_SOCKET, option,
             reinterpret_cast<const char*>(&ms), sizeof(ms));
#else
  struct timeval tv;
  tv.tv_sec  = timeout.count() / 1000;
  tv.tv_usec = (timeout.count() % 1000) * 1000;
  setsockopt(sock, SOL_SOCKET, option, &tv, sizeof(tv));
#endif
}

}  // namespace

// --- Construction / destruction ----------------------------------------

#ifdef _WIN32
TcpSocket::TcpSocket() {
  WSADATA wsa_data;
  wsa_initialized_ = (WSAStartup(MAKEWORD(2, 2), &wsa_data) == 0);
}
#else
TcpSocket::TcpSocket() = default;
#endif

TcpSocket::~TcpSocket() {
  Close();
#ifdef _WIN32
  if (wsa_initialized_) {
    WSACleanup();
    wsa_initialized_ = false;
  }
#endif
}

TcpSocket::TcpSocket(TcpSocket&& other) noexcept
    : socket_(other.socket_),
      connected_(other.connected_),
      read_buffer_(std::move(other.read_buffer_))
#ifdef _WIN32
      ,
      wsa_initialized_(other.wsa_initialized_)
#endif
{
  other.socket_    = kInvalidNativeSocket;
  other.connected_ = false;
#ifdef _WIN32
  other.wsa_initialized_ = false;
#endif
}

TcpSocket& TcpSocket::operator=(TcpSocket&& other) noexcept {
  if (this != &other) {
    Close();
#ifdef _WIN32
    if (wsa_initialized_) {
      WSACleanup();
    }
    wsa_initialized_       = other.wsa_initialized_;
    other.wsa_initialized_ = false;
#endif
    socket_         = other.socket_;
    connected_      = other.connected_;
    read_buffer_    = std::move(other.read_buffer_);
    other.socket_   = kInvalidNativeSocket;
    other.connected_ = false;
  }
  return *this;
}

// --- Connect -----------------------------------------------------------

std::expected<void, XpsError> TcpSocket::Connect(
    std::string_view host, uint16_t port,
    std::chrono::milliseconds timeout) {
#ifdef _WIN32
  if (!wsa_initialized_) {
    return std::unexpected(
        XpsError{-1, "Connect", "Winsock initialisation failed"});
  }
#endif

  Close();  // discard any prior connection

  // Resolve host to an address.
  struct addrinfo hints = {};
  hints.ai_family   = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  const std::string port_str = std::to_string(port);
  const std::string host_str(host);
  struct addrinfo* result = nullptr;

  if (getaddrinfo(host_str.c_str(), port_str.c_str(), &hints, &result) != 0) {
    return std::unexpected(LastSocketError("getaddrinfo"));
  }

  socket_ = ::socket(result->ai_family, result->ai_socktype,
                     result->ai_protocol);
  if (socket_ == kInvalidNativeSocket) {
    freeaddrinfo(result);
    return std::unexpected(LastSocketError("socket"));
  }

  // Switch to non-blocking mode so connect() returns immediately and we can
  // wait with select() for at most `timeout`.
#ifdef _WIN32
  u_long nonblocking = 1;
  ioctlsocket(socket_, FIONBIO, &nonblocking);
#else
  int flags = fcntl(socket_, F_GETFL, 0);
  fcntl(socket_, F_SETFL, flags | O_NONBLOCK);
#endif

  const int connect_rc =
      ::connect(socket_, result->ai_addr,
                static_cast<socklen_t>(result->ai_addrlen));
  freeaddrinfo(result);

#ifdef _WIN32
  const bool in_progress =
      (connect_rc == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK);
#else
  const bool in_progress = (connect_rc == -1 && errno == EINPROGRESS);
#endif

  if (connect_rc != 0 && !in_progress) {
    auto err = LastSocketError("connect");
    Close();
    return std::unexpected(err);
  }

  if (in_progress) {
    // Wait for the socket to become writable (connected) or for timeout.
    fd_set write_fds;
    FD_ZERO(&write_fds);
    FD_SET(socket_, &write_fds);

    struct timeval tv;
    tv.tv_sec  = static_cast<long>(timeout.count() / 1000);
    tv.tv_usec = static_cast<long>((timeout.count() % 1000) * 1000);

    // Note: first arg to select is ignored on Windows; correct on POSIX.
    const int sel = ::select(static_cast<int>(socket_ + 1),
                             nullptr, &write_fds, nullptr, &tv);
    if (sel == 0) {
      Close();
      return std::unexpected(
          XpsError{-1, "connect", "Connection timed out"});
    }
    if (sel < 0) {
      auto err = LastSocketError("select");
      Close();
      return std::unexpected(err);
    }

    // select() returned writable — check SO_ERROR to confirm success.
    int so_error = 0;
    socklen_t len = sizeof(so_error);
    getsockopt(socket_, SOL_SOCKET, SO_ERROR,
               reinterpret_cast<char*>(&so_error), &len);
    if (so_error != 0) {
      // Connection was refused or reset — surface the underlying error code.
      Close();
      return std::unexpected(
          XpsError{so_error, "connect",
                   "Connection failed (SO_ERROR=" + std::to_string(so_error) +
                       ")"});
    }
  }

  // Restore blocking mode for subsequent send/recv calls.
#ifdef _WIN32
  u_long blocking = 0;
  ioctlsocket(socket_, FIONBIO, &blocking);
#else
  int flags2 = fcntl(socket_, F_GETFL, 0);
  fcntl(socket_, F_SETFL, flags2 & ~O_NONBLOCK);
#endif

  connected_ = true;
  return {};
}

// --- Send --------------------------------------------------------------

std::expected<void, XpsError> TcpSocket::Send(std::string_view data) {
  if (!connected_) {
    return std::unexpected(XpsError{-1, "Send", "Not connected"});
  }

  const char* ptr       = data.data();
  int         remaining = static_cast<int>(data.size());

  while (remaining > 0) {
#ifdef _WIN32
    const int sent = ::send(socket_, ptr, remaining, 0);
    if (sent == SOCKET_ERROR) {
#else
    const ssize_t sent = ::send(socket_, ptr, remaining, 0);
    if (sent < 0) {
#endif
      connected_ = false;
      return std::unexpected(LastSocketError("send"));
    }
    ptr       += sent;
    remaining -= static_cast<int>(sent);
  }
  return {};
}

// --- ReadUntil ---------------------------------------------------------

std::expected<std::string, XpsError> TcpSocket::ReadUntil(
    std::string_view sentinel, std::chrono::milliseconds timeout) {
  if (!connected_) {
    return std::unexpected(XpsError{-1, "ReadUntil", "Not connected"});
  }

  // Check if the sentinel is already in the carry-over buffer from a prior
  // call (e.g. two XPS responses arrived in a single recv()).
  auto pos = read_buffer_.find(sentinel);
  if (pos != std::string::npos) {
    const size_t end = pos + sentinel.size();
    std::string result = read_buffer_.substr(0, end);
    read_buffer_.erase(0, end);
    return result;
  }

  SetSocketTimeout(socket_, SO_RCVTIMEO, timeout);

  char buf[4096];
  while (true) {
#ifdef _WIN32
    const int received = ::recv(socket_, buf, sizeof(buf), 0);
    if (received == SOCKET_ERROR) {
      const int err_code = WSAGetLastError();
      if (err_code == WSAETIMEDOUT) {
        return std::unexpected(
            XpsError{-1, "ReadUntil", "Receive timed out"});
      }
      connected_ = false;
      return std::unexpected(LastSocketError("recv"));
    }
#else
    const ssize_t received = ::recv(socket_, buf, sizeof(buf), 0);
    if (received < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return std::unexpected(
            XpsError{-1, "ReadUntil", "Receive timed out"});
      }
      connected_ = false;
      return std::unexpected(LastSocketError("recv"));
    }
#endif
    if (received == 0) {
      connected_ = false;
      return std::unexpected(
          XpsError{-1, "ReadUntil", "Connection closed by remote host"});
    }

    read_buffer_.append(buf, static_cast<size_t>(received));

    pos = read_buffer_.find(sentinel);
    if (pos != std::string::npos) {
      const size_t end = pos + sentinel.size();
      std::string result = read_buffer_.substr(0, end);
      read_buffer_.erase(0, end);
      return result;
    }
  }
}

// --- Close -------------------------------------------------------------

void TcpSocket::Close() {
  if (socket_ != kInvalidNativeSocket) {
#ifdef _WIN32
    ::shutdown(socket_, SD_BOTH);
    ::closesocket(socket_);
#else
    ::shutdown(socket_, SHUT_RDWR);
    ::close(socket_);
#endif
    socket_ = kInvalidNativeSocket;
  }
  connected_ = false;
  read_buffer_.clear();
}

bool TcpSocket::IsConnected() const { return connected_; }

}  // namespace newport::xps::internal
