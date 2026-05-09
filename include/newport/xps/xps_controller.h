#ifndef NEWPORT_XPS_XPS_CONTROLLER_H_
#define NEWPORT_XPS_XPS_CONTROLLER_H_

#include <chrono>
#include <expected>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

#include "newport/xps/xps_export.h"

namespace newport::xps {

class NEWPORT_XPS_API XpsController {
 public:
  explicit XpsController();
  ~XpsController();

  XpsController(const XpsController&) = delete;
  XpsController& operator=(const XpsController&) = delete;
  XpsController(XpsController&&) noexcept;
  XpsController& operator=(XpsController&&) noexcept;

  /// @brief Create and open a socket.
  /// @details This function is used to create and open a socket. Send Timeout
  /// is set to 1 second
  /// @param ipAddress IP Address of instrument.
  /// @param port Port number.
  /// @param timeout ReceiveTimeout in milliseconds
  /// @return Error code 0: No error. -1: Error of socket open.
  [[nodiscard]] int OpenInstrument(std::string ipAddress, int port,
                                   int timeout = 1000);

  /// @brief Closes the current socket.
  /// @details This function is used to close the current socket.
  /// @return Error code 0: No error. -1: Error of socket close.
  [[nodiscard]] int CloseInstrument();

  /// @brief Configures socket timeout.
  /// @details This function is used to configure socket timeout for sending and
  /// reading.
  /// @param sendingTimeout Sending timeout in milliseconds
  /// @param readingTimeout Reading timeout in milliseconds
  /// @return Error code 0: No error. -1: Error of set timeout.
  [[nodiscard]] int SetTimeout(int sendingTimeout, int readingTimeout);

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace newport::xps

#endif  // NEWPORT_XPS_XPS_CONTROLLER_H_
