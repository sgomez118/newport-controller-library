#include <array>

#include <gtest/gtest.h>

#include "newport/xps/xps_controller.h"
#include "newport/xps/xps_group.h"
#include "newport/xps/xps_positioner.h"

using newport::xps::ConnectionConfig;
using newport::xps::XpsController;

namespace {

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

// --- Group / Positioner handle relationships (no network required) ---

TEST(GroupTest, GetPositionerBuildsFullName) {
  XpsController controller(MakeTestConfig());
  auto group      = controller.GetGroup("XYZR");
  auto positioner = group.GetPositioner("X");
  EXPECT_EQ(positioner.name(), "XYZR.X");
}

TEST(GroupTest, GetPositionerFromControllerPreservesName) {
  XpsController controller(MakeTestConfig());
  auto positioner = controller.GetPositioner("STAGE1");
  EXPECT_EQ(positioner.name(), "STAGE1");
}

// --- Stub behaviour (Phase 1d: replace with MockXpsServer-backed tests) ---

TEST(GroupTest, InitializeReturnsErrorBeforeImplemented) {
  XpsController controller(MakeTestConfig());
  auto group  = controller.GetGroup("XYZR");
  auto result = group.Initialize();
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code, -1);
}

TEST(GroupTest, MoveAbsoluteReturnsErrorBeforeImplemented) {
  XpsController controller(MakeTestConfig());
  auto group  = controller.GetGroup("XYZR");
  const std::array<double, 4> positions = {1.0, 2.0, 3.0, 4.0};
  auto result = group.MoveAbsolute(positions);
  EXPECT_FALSE(result.has_value());
}

TEST(GroupTest, JogEnableReturnsErrorBeforeImplemented) {
  XpsController controller(MakeTestConfig());
  auto group  = controller.GetGroup("XYZR");
  auto result = group.JogEnable(/*velocity=*/5.0, /*acceleration=*/100.0);
  EXPECT_FALSE(result.has_value());
}

TEST(PositionerTest, MoveAbsoluteReturnsErrorBeforeImplemented) {
  XpsController controller(MakeTestConfig());
  auto positioner = controller.GetPositioner("STAGE1");
  auto result     = positioner.MoveAbsolute(10.0);
  EXPECT_FALSE(result.has_value());
}
