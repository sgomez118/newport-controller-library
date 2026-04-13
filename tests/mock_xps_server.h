#ifndef TESTS_MOCK_XPS_SERVER_H_
#define TESTS_MOCK_XPS_SERVER_H_

#include <atomic>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

// A minimal TCP server that accepts one connection and responds to XPS
// commands with scripted responses.  Used by unit tests to exercise the
// library without a physical XPS controller.
//
// Expectations are matched in order.  Each Expect() call enqueues one
// command/response pair; the serve loop reads the next command from the
// socket, asserts it matches the expected string, then writes back the
// response.
//
// Usage:
//   MockXpsServer server;
//   server.Expect("Login(Administrator,Administrator)\n", "0,EndOfAPI");
//   server.Expect("FirmwareVersionGet()\n", "0,XPS-D v2.8.x,EndOfAPI");
//
//   uint16_t port = server.Start();  // binds to a free loopback port
//
//   // ... run library code against 127.0.0.1:port ...
//
//   server.Stop();
//   server.VerifyAllExpectationsMatched();  // GTest EXPECT inside
class MockXpsServer {
 public:
  struct Expectation {
    std::string command;   // raw bytes the client is expected to send (incl. \n)
    std::string response;  // bytes to write back (must end with "EndOfAPI")
  };

  MockXpsServer() = default;
  ~MockXpsServer();

  MockXpsServer(const MockXpsServer&)            = delete;
  MockXpsServer& operator=(const MockXpsServer&) = delete;

  // Enqueues a command/response pair.  Call before Start().
  void Expect(std::string command, std::string response);

  // Binds to a free loopback port, starts the serve loop on a background
  // thread, and returns the bound port number.  Blocks until the socket is
  // bound and listening so the caller can connect immediately after returning.
  [[nodiscard]] uint16_t Start();

  // Closes the listen socket (which unblocks any pending accept/recv) and
  // joins the background thread.
  void Stop();

  // Fails the test (via GTest EXPECT_EQ) if any enqueued expectations were
  // not consumed.
  void VerifyAllExpectationsMatched();

 private:
  void ServeLoop();

  std::vector<Expectation> expectations_;
  size_t                   next_expectation_ = 0;

  // intptr_t holds both a POSIX int fd and a Windows SOCKET (UINT_PTR) with
  // -1 / INVALID_SOCKET (all-bits-set) as the invalid sentinel.
  intptr_t listen_fd_ = -1;
  intptr_t client_fd_ = -1;

  uint16_t              port_    = 0;
  std::thread           thread_;
  std::atomic<bool>     running_ = false;
};

#endif  // TESTS_MOCK_XPS_SERVER_H_
