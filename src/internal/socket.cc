#include "socket.h"

#include <string>
#include <string_view>

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
using socket_handle_t = SOCKET;
using socklen_t_compat = int;
static constexpr socket_handle_t kInvalidSocket = INVALID_SOCKET;
static constexpr int kSocketError = SOCKET_ERROR;
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

}  // namespace newport::xps::internal
