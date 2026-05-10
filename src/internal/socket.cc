#include "socket.h"

#include <string>

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
}  // namespace

}  // namespace newport::xps::internal
