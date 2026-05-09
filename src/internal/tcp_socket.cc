#include "internal/tcp_socket.h"

#include <string>

namespace newport::xps::internal {
TcpSocket::TcpSocket() {
#ifdef _WIN32
  WSADATA wsa_data;
  wsa_initialized_ = (WSAStartup(MAKEWORD(2, 2), &wsa_data) == 0);
#endif
}

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
  other.socket_ = kInvalidNativeSocket;
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
    wsa_initialized_ = other.wsa_initialized_;
    other.wsa_initialized_ = false;
#endif
    socket_ = other.socket_;
    connected_ = other.connected_;
    read_buffer_ = std::move(other.read_buffer_);
    other.socket_ = kInvalidNativeSocket;
    other.connected_ = false;
  }
  return *this;
}

int TcpSocket::Connect(std::string_view host, uint16_t port,
                       std::chrono::milliseconds timeout) {
  if (connected_) {
    return 0;
  }

  addrinfo hints{};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  addrinfo* results = nullptr;
  const std::string host_str(host);
  const std::string port_str = std::to_string(port);

  if (int ret =
          getaddrinfo(host_str.c_str(), port_str.c_str(), &hints, &results);
      ret != 0) {
    return ret;
  }

  for (addrinfo* addr = results; addr != nullptr; addr = addr->ai_next) {
    socket_ = ::socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
    if (socket_ == kInvalidNativeSocket) {
      continue;
    }

#ifdef _WIN32
    u_long nb = 1;
    ioctlsocket(socket_, FIONBIO, &nb);
#else
    int flags = fcntl(socket_, F_GETFL, 0);
    fcntl(socket_, F_SETFL, flags | O_NONBLOCK);
#endif

    int ret =
        ::connect(socket_, addr->ai_addr, static_cast<int>(addr->ai_addrlen));
    bool succeeded = (ret == 0);

    if (!succeeded) {
#ifdef _WIN32
      const bool in_progress = (WSAGetLastError() == WSAEWOULDBLOCK);
#else
      const bool in_progress = (errno == EINPROGRESS);
#endif
      if (in_progress) {
        auto secs = std::chrono::duration_cast<std::chrono::seconds>(timeout);
        auto usecs = std::chrono::duration_cast<std::chrono::microseconds>(
            timeout - secs);
        timeval tv{};
        tv.tv_sec = static_cast<long>(secs.count());
        tv.tv_usec = static_cast<long>(usecs.count());

        fd_set write_fds;
        FD_ZERO(&write_fds);
        FD_SET(socket_, &write_fds);

        if (select(static_cast<int>(socket_) + 1, nullptr, &write_fds, nullptr,
                   &tv) > 0) {
          int error = 0;
          socklen_t len = sizeof(error);
          getsockopt(socket_, SOL_SOCKET, SO_ERROR,
                     reinterpret_cast<char*>(&error), &len);
          succeeded = (error == 0);
        }
      }
    }

    if (succeeded) {
      connected_ = true;
      break;
    }

#ifdef _WIN32
    closesocket(socket_);
#else
    ::close(socket_);
#endif
    socket_ = kInvalidNativeSocket;
  }

  freeaddrinfo(results);

  if (!connected_) {
    return -1;
  }

  // Restore blocking mode for Send/Read
#ifdef _WIN32
  u_long nb = 0;
  ioctlsocket(socket_, FIONBIO, &nb);
#else
  int flags = fcntl(socket_, F_GETFL, 0);
  fcntl(socket_, F_SETFL, flags & ~O_NONBLOCK);
#endif

  return 0;
}

int TcpSocket::Send(std::string_view data) {
  if (!connected_) {
    return -1;
  }

  const char* ptr = data.data();
  size_t remaining = data.size();

  while (remaining > 0) {
#ifdef _WIN32
    int n = ::send(socket_, ptr, static_cast<int>(remaining), 0);
    if (n == SOCKET_ERROR) {
      connected_ = false;
      return WSAGetLastError();
    }
#else
    ssize_t n = ::send(socket_, ptr, remaining, 0);
    if (n < 0) {
      connected_ = false;
      return errno;
    }
#endif
    ptr += n;
    remaining -= static_cast<size_t>(n);
  }

  return 0;
}

int TcpSocket::Receive(std::chrono::milliseconds timeout) {
  if (!connected_) {
    return -1;
  }

  const auto secs =
      std::chrono::duration_cast<std::chrono::seconds>(timeout);
  const auto usecs =
      std::chrono::duration_cast<std::chrono::microseconds>(timeout - secs);
  timeval tv{};
  tv.tv_sec = static_cast<long>(secs.count());
  tv.tv_usec = static_cast<long>(usecs.count());

  fd_set read_fds;
  FD_ZERO(&read_fds);
  FD_SET(socket_, &read_fds);

  const int ready =
      select(static_cast<int>(socket_) + 1, &read_fds, nullptr, nullptr, &tv);
  if (ready == 0) {
    return -1;
  }
  if (ready < 0) {
    connected_ = false;
    return -1;
  }

  char buf[4096];
#ifdef _WIN32
  const int n = ::recv(socket_, buf, static_cast<int>(sizeof(buf)), 0);
  if (n == SOCKET_ERROR || n == 0) {
    connected_ = false;
    return (n == SOCKET_ERROR) ? WSAGetLastError() : -1;
  }
#else
  const ssize_t n = ::recv(socket_, buf, sizeof(buf), 0);
  if (n <= 0) {
    connected_ = false;
    return (n < 0) ? errno : -1;
  }
#endif
  read_buffer_.append(buf, static_cast<size_t>(n));

  return 0;
}

void TcpSocket::Close() {
  if (socket_ != kInvalidNativeSocket) {
#ifdef _WIN32
    closesocket(socket_);
#else
    ::close(socket_);
#endif
    socket_ = kInvalidNativeSocket;
  }
  connected_ = false;
  read_buffer_.clear();
}

bool TcpSocket::IsConnected() const { return connected_; }

}  // namespace newport::xps::internal
