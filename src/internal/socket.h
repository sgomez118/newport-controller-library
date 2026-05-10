#ifndef NEWPORT_XPS_INTERNAL_SOCKET_H_
#define NEWPORT_XPS_INTERNAL_SOCKET_H_

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "newport/xps/error.h"

namespace newport::xps::internal {

class Socket {
 public:
  static constexpr std::uintptr_t kInvalidHandle =
      static_cast<std::uintptr_t>(-1);

  [[nodiscard]] static Result<Socket> Connect(
      std::string_view host, int port,
      std::chrono::milliseconds connectTimeout = std::chrono::seconds{5});

  Socket() noexcept = default;
  ~Socket();

  // Move-only: copyig a socket handle would lead to double-close
  Socket(const Socket&) = delete;
  Socket& operator=(const Socket&) = delete;

  Socket(Socket&& other) noexcept;
  Socket& operator=(Socket&& other) noexcept;

  [[nodiscard]] bool IsOpen() const noexcept {
    return handle_ != kInvalidHandle;
  }

  void Close() noexcept;

  [[nodiscard]] Result<void> SendAll(std::span<const std::byte> data);

  [[nodiscard]] Result<std::size_t> RecvSome(std::span<std::byte> out);

  [[nodiscard]] Result<void> SetSendTimeout(std::chrono::milliseconds timeout);

  [[nodiscard]] Result<void> SetRecvTimeout(std::chrono::milliseconds timeout);

  [[nodiscard]] std::uintptr_t native_handle() const noexcept {
    return handle_;
  }

 private:
  explicit Socket(std::uintptr_t h) noexcept : handle_(h) {};

  std::uintptr_t handle_ = kInvalidHandle;
};

}  // namespace newport::xps::internal

#endif  // NEWPORT_XPS_INTERNAL_SOCKET_H_
