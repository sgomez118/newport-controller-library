#include "newport/xps/xps_controller.h"

#include "internal/tcp_socket.h"

namespace newport::xps {

struct XpsController::Impl {
  internal::TcpSocket socket;
  std::chrono::milliseconds send_timeout{1000};
  std::chrono::milliseconds receive_timeout{1000};
};

XpsController::XpsController() : impl_(std::make_unique<Impl>()) {}

XpsController::~XpsController() = default;

XpsController::XpsController(XpsController&&) noexcept = default;
XpsController& XpsController::operator=(XpsController&&) noexcept = default;

int XpsController::OpenInstrument(std::string ipAddress, int port,
                                  int timeout) {
  impl_->receive_timeout = std::chrono::milliseconds(timeout);
  return impl_->socket.Connect(ipAddress, static_cast<uint16_t>(port),
                               impl_->receive_timeout);
}

int XpsController::CloseInstrument() {
  impl_->socket.Close();
  return impl_->socket.IsConnected() ? -1 : 0;
}

int XpsController::SetTimeout(int sendingTimeout, int readingTimeout) {
  impl_->send_timeout = std::chrono::milliseconds(sendingTimeout);
  impl_->receive_timeout = std::chrono::milliseconds(readingTimeout);
  return 0;
}

}  // namespace newport::xps