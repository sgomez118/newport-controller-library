#include <gtest/gtest.h>

#include "newport/xps/xps_controller.h"

using newport::xps::ConnectionConfig;
using newport::xps::XpsController;

namespace {

// Returns a config pointing at a loopback address with no retries so tests
// fail fast rather than waiting on reconnect delays.
ConnectionConfig MakeTestConfig(uint16_t port = 9999) {
  ConnectionConfig cfg;
  cfg.host                   = "127.0.0.1";
  cfg.port                   = port;
  cfg.username               = "Administrator";
  cfg.password               = "Administrator";
  cfg.connect_timeout        = std::chrono::milliseconds(200);
  cfg.command_timeout        = std::chrono::milliseconds(200);
  cfg.max_reconnect_attempts = 0;
  return cfg;
}

}  // namespace

// --- State before Connect() ---

TEST(XpsControllerTest, IsDisconnectedBeforeConnect) {
  XpsController controller(MakeTestConfig());
  EXPECT_FALSE(controller.IsConnected());
}

// --- Factory methods (no network required) ---

TEST(XpsControllerTest, GetGroupReturnsCorrectName) {
  XpsController controller(MakeTestConfig());
  auto group = controller.GetGroup("XYZR");
  EXPECT_EQ(group.name(), "XYZR");
}

TEST(XpsControllerTest, GetPositionerReturnsCorrectName) {
  XpsController controller(MakeTestConfig());
  auto pos = controller.GetPositioner("XYZR.X");
  EXPECT_EQ(pos.name(), "XYZR.X");
}

// --- Connect against a real server (Phase 1c: expand with MockXpsServer) ---

TEST(XpsControllerTest, ConnectFailsWhenNoServerListening) {
  XpsController controller(MakeTestConfig(/*port=*/19999));
  auto result = controller.Connect();
  EXPECT_FALSE(result.has_value());
  EXPECT_FALSE(controller.IsConnected());
}
