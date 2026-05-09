#include "newport/xps/controller.h"

#include "internal/tcp_socket.h"

namespace newport::xps {

struct Controller::Impl {
  internal::TcpSocket socket;
  std::chrono::milliseconds send_timeout{1000};
  std::chrono::milliseconds receive_timeout{1000};
};

Controller::Controller() : impl_(std::make_unique<Impl>()) {}

Controller::~Controller() = default;

Controller::Controller(Controller&&) noexcept = default;
Controller& Controller::operator=(Controller&&) noexcept = default;

int Controller::OpenInstrument(std::string ipAddress, int port, int timeout) {
  impl_->receive_timeout = std::chrono::milliseconds(timeout);
  return impl_->socket.Connect(ipAddress, static_cast<uint16_t>(port),
                               impl_->receive_timeout);
}

int Controller::CloseInstrument() {
  impl_->socket.Close();
  return impl_->socket.IsConnected() ? -1 : 0;
}

int Controller::SetTimeout(int sendingTimeout, int readingTimeout) {
  impl_->send_timeout = std::chrono::milliseconds(sendingTimeout);
  impl_->receive_timeout = std::chrono::milliseconds(readingTimeout);
  return 0;
}

}  // namespace newport::xps