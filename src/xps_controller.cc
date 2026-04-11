#include "newport/xps/xps_controller.h"

#include <utility>

#include "newport/xps/xps_group.h"
#include "newport/xps/xps_positioner.h"

namespace newport::xps {

namespace {

XpsError NotImplemented(std::string_view command) {
  return XpsError{-1, std::string(command), "Not implemented"};
}

}  // namespace

XpsController::XpsController(ConnectionConfig config)
    : config_(std::move(config)) {}

XpsController::~XpsController() { Disconnect(); }

std::expected<void, XpsError> XpsController::Connect() {
  // Phase 1c: open TCP socket, send Login command, start reconnect watchdog.
  return std::unexpected(NotImplemented("Login"));
}

void XpsController::Disconnect() {
  // Phase 1c: close socket, cancel reconnect watchdog.
  connected_ = false;
}

bool XpsController::IsConnected() const { return connected_; }

void XpsController::SetOnReconnect(std::function<void()> callback) {
  on_reconnect_ = std::move(callback);
}

void XpsController::SetOnDisconnect(std::function<void()> callback) {
  on_disconnect_ = std::move(callback);
}

Group XpsController::GetGroup(std::string_view name) {
  return Group(this, std::string(name));
}

Positioner XpsController::GetPositioner(std::string_view name) {
  return Positioner(this, std::string(name));
}

std::expected<std::string, XpsError> XpsController::GetFirmwareVersion() {
  // Phase 1c: send "FirmwareVersionGet()" and parse response.
  return std::unexpected(NotImplemented("FirmwareVersionGet"));
}

}  // namespace newport::xps
