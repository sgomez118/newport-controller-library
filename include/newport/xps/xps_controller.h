#ifndef NEWPORT_XPS_XPS_CONTROLLER_H_
#define NEWPORT_XPS_XPS_CONTROLLER_H_

#include <chrono>
#include <expected>
#include <functional>
#include <string>
#include <string_view>

#include "newport/xps/xps_error.h"
#include "newport/xps/xps_export.h"
#include "newport/xps/xps_group.h"
#include "newport/xps/xps_positioner.h"

namespace newport::xps {

// Parameters for establishing and maintaining the TCP connection to the XPS.
// Construct one of these and pass it to XpsController.  All fields with
// defaults are reasonable for a local-network XPS-D; adjust timeouts for
// high-latency or unreliable links.
struct ConnectionConfig {
  std::string host;      // IP address or hostname of the XPS controller
  uint16_t    port = 5001;

  // Credentials — the XPS ships with Administrator/Administrator by default.
  // Users are responsible for configuring their own credentials.
  std::string username;
  std::string password;

  // How long to wait for the TCP handshake and XPS login to complete.
  std::chrono::milliseconds connect_timeout = std::chrono::seconds(5);

  // How long to wait for a response to a single command before treating the
  // connection as dead and initiating a reconnection attempt.
  std::chrono::milliseconds command_timeout = std::chrono::seconds(10);

  // Maximum number of automatic reconnection attempts after a connection drop.
  // Set to 0 to disable automatic reconnection entirely.
  int max_reconnect_attempts = 3;

  // Delay before the first reconnect attempt.  Subsequent attempts use
  // exponential backoff capped at 4× this value.
  std::chrono::milliseconds reconnect_delay = std::chrono::milliseconds(500);
};

// Manages a single TCP connection to a Newport XPS-D motion controller.
//
// Threading model: one XpsController = one socket = one thread.  Methods are
// not thread-safe.  For concurrent access create multiple XpsController
// instances — the XPS-D supports up to 100 simultaneous connections on
// port 5001.
//
// Typical usage:
//
//   ConnectionConfig cfg;
//   cfg.host     = "192.168.0.254";
//   cfg.username = "Administrator";
//   cfg.password = "Administrator";
//
//   XpsController controller(cfg);
//   controller.SetOnReconnect([] { /* restart jog or motion if needed */ });
//
//   if (auto err = controller.Connect(); !err) {
//     // err.error() has .code, .command, and .message
//     return;
//   }
//
//   auto stage = controller.GetGroup("XYZR");
//   stage.Initialize();
//   stage.HomeSearch();
//   stage.MoveAbsolute({0.0, 0.0, 0.0, 0.0});
class NEWPORT_XPS_API XpsController {
 public:
  explicit XpsController(ConnectionConfig config);
  ~XpsController();

  XpsController(const XpsController&)            = delete;
  XpsController& operator=(const XpsController&) = delete;
  XpsController(XpsController&&)                 = default;
  XpsController& operator=(XpsController&&)      = default;

  // --- Connection -------------------------------------------------------

  // Establishes the TCP connection and logs in to the XPS controller.
  // Must be called before any other method.  Respects connect_timeout.
  [[nodiscard]] std::expected<void, XpsError> Connect();

  // Closes the connection gracefully.  Safe to call when already disconnected.
  // Called automatically by the destructor.
  void Disconnect();

  [[nodiscard]] bool IsConnected() const;

  // --- Reconnection callbacks -------------------------------------------

  // Invoked on the controller's owning thread when a dropped connection is
  // successfully re-established.  Any active jog or motion must be restarted
  // manually — the XPS will have stopped the stage on its side.
  void SetOnReconnect(std::function<void()> callback);

  // Invoked on the controller's owning thread when the connection is lost and
  // all reconnect attempts have been exhausted.
  void SetOnDisconnect(std::function<void()> callback);

  // --- Group / Positioner factory ---------------------------------------

  // Returns a Group handle for the named XPS group.
  // Does not communicate with the controller — always succeeds.
  [[nodiscard]] Group GetGroup(std::string_view name);

  // Returns a Positioner handle.  Use for single-axis XPS groups (e.g.
  // "STAGE1") or for per-axis queries within a multi-axis group (e.g.
  // "XYZR.X").  Does not communicate with the controller — always succeeds.
  [[nodiscard]] Positioner GetPositioner(std::string_view name);

  // --- System -----------------------------------------------------------

  // Returns the controller firmware version string.
  [[nodiscard]] std::expected<std::string, XpsError> GetFirmwareVersion();

 private:
  ConnectionConfig config_;
  bool             connected_ = false;

  std::function<void()> on_reconnect_;
  std::function<void()> on_disconnect_;
};

}  // namespace newport::xps

#endif  // NEWPORT_XPS_XPS_CONTROLLER_H_
