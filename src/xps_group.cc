#include "newport/xps/xps_group.h"

#include <string>
#include <utility>

// XpsController is forward-declared in xps_group.h; the full definition is
// needed here once the protocol layer is wired in (Phase 1d).
#include "newport/xps/xps_controller.h"

namespace newport::xps {

namespace {

XpsError NotImplemented(std::string_view command) {
  return XpsError{-1, std::string(command), "Not implemented"};
}

}  // namespace

Group::Group(XpsController* controller, std::string name)
    : controller_(controller), name_(std::move(name)) {}

std::string_view Group::name() const { return name_; }

Positioner Group::GetPositioner(std::string_view positioner_name) {
  return Positioner(controller_, name_ + "." + std::string(positioner_name));
}

std::expected<void, XpsError> Group::Initialize() {
  // Phase 1d: send "GroupInitialize(<name>)".
  return std::unexpected(NotImplemented("GroupInitialize"));
}

std::expected<void, XpsError> Group::HomeSearch() {
  // Phase 1d: send "GroupHomeSearch(<name>)".
  return std::unexpected(NotImplemented("GroupHomeSearch"));
}

std::expected<void, XpsError> Group::Kill() {
  // Phase 1d: send "GroupKill(<name>)".
  return std::unexpected(NotImplemented("GroupKill"));
}

std::expected<void, XpsError> Group::MoveAbsolute(
    std::span<const double> positions) {
  // Phase 1d: send "GroupMoveAbsolute(<name>,<p0>,<p1>,...)".
  return std::unexpected(NotImplemented("GroupMoveAbsolute"));
}

std::expected<void, XpsError> Group::MoveRelative(
    std::span<const double> displacements) {
  // Phase 1d: send "GroupMoveRelative(<name>,<d0>,<d1>,...)".
  return std::unexpected(NotImplemented("GroupMoveRelative"));
}

std::expected<void, XpsError> Group::Stop() {
  // Phase 1d: send "GroupMoveAbort(<name>)".
  return std::unexpected(NotImplemented("GroupMoveAbort"));
}

std::expected<void, XpsError> Group::JogEnable(double velocity,
                                                double acceleration) {
  // Phase 1d: send "GroupJogParametersSet" then "GroupJogModeEnable".
  return std::unexpected(NotImplemented("GroupJogModeEnable"));
}

std::expected<void, XpsError> Group::JogDisable() {
  // Phase 1d: send "GroupJogModeDisable(<name>)".
  return std::unexpected(NotImplemented("GroupJogModeDisable"));
}

std::expected<std::vector<double>, XpsError> Group::GetCurrentPosition() {
  // Phase 1d: send "GroupCurrentPositionGet(<name>,<n_positioners>)".
  return std::unexpected(NotImplemented("GroupCurrentPositionGet"));
}

std::expected<GroupStatus, XpsError> Group::GetStatus() {
  // Phase 1d: send "GroupStatusGet(<name>)" and map integer to GroupStatus.
  return std::unexpected(NotImplemented("GroupStatusGet"));
}

}  // namespace newport::xps
