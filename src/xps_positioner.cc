#include "newport/xps/xps_positioner.h"

#include <string>
#include <utility>

// XpsController is forward-declared in xps_positioner.h; the full definition
// is needed here once the protocol layer is wired in (Phase 1d).
#include "newport/xps/xps_controller.h"

namespace newport::xps {

namespace {

XpsError NotImplemented(std::string_view command) {
  return XpsError{-1, std::string(command), "Not implemented"};
}

}  // namespace

Positioner::Positioner(XpsController* controller, std::string name)
    : controller_(controller), name_(std::move(name)) {}

std::string_view Positioner::name() const { return name_; }

std::expected<void, XpsError> Positioner::MoveAbsolute(double position) {
  // Phase 1d: send "GroupMoveAbsolute(<name>,<position>)".
  // For single-axis groups the group name and positioner name are the same.
  return std::unexpected(NotImplemented("GroupMoveAbsolute"));
}

std::expected<void, XpsError> Positioner::MoveRelative(double displacement) {
  // Phase 1d: send "GroupMoveRelative(<name>,<displacement>)".
  return std::unexpected(NotImplemented("GroupMoveRelative"));
}

std::expected<double, XpsError> Positioner::GetCurrentPosition() {
  // Phase 1d: send "PositionerCurrentPositionGet(<name>)".
  return std::unexpected(NotImplemented("PositionerCurrentPositionGet"));
}

std::expected<int, XpsError> Positioner::GetHardwareStatus() {
  // Phase 1d: send "PositionerHardwareStatusGet(<name>)".
  return std::unexpected(NotImplemented("PositionerHardwareStatusGet"));
}

}  // namespace newport::xps
