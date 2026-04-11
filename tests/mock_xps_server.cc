#include "mock_xps_server.h"

#include <gtest/gtest.h>

// Phase 1c: implement TCP listen/accept/serve loop.
// The full implementation will use platform sockets (Winsock2 on Windows,
// POSIX sockets on Linux/macOS) to bind a loopback listener, accept one
// client connection, and dispatch incoming XPS command strings to the
// scripted expectation list.

MockXpsServer::~MockXpsServer() { Stop(); }

void MockXpsServer::Expect(std::string command, std::string response) {
  expectations_.push_back({std::move(command), std::move(response)});
}

uint16_t MockXpsServer::Start() {
  // TODO(Phase 1c): bind loopback socket, spawn ServeLoop thread.
  return port_;
}

void MockXpsServer::Stop() {
  // TODO(Phase 1c): set running_ = false, close sockets, join thread_.
  running_ = false;
}

void MockXpsServer::VerifyAllExpectationsMatched() {
  EXPECT_EQ(next_expectation_, expectations_.size())
      << next_expectation_ << " of " << expectations_.size()
      << " MockXpsServer expectations were consumed";
}

void MockXpsServer::ServeLoop() {
  // TODO(Phase 1c): accept client_fd_, loop over expectations_:
  //   1. Read bytes until '\n'.
  //   2. EXPECT_EQ(received, expectations_[next_expectation_].command).
  //   3. Write expectations_[next_expectation_].response to client.
  //   4. ++next_expectation_.
}
