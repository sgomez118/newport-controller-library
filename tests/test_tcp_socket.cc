#include "internal/tcp_socket.h"

#include <array>

#include <gtest/gtest.h>

#include "mock_xps_server.h"

using newport::xps::internal::TcpSocket;

// Helper: short timeouts so tests fail fast rather than hanging.
constexpr auto kConnectTimeout = std::chrono::milliseconds(500);
constexpr auto kReadTimeout    = std::chrono::milliseconds(500);

// ---------------------------------------------------------------------------
// Connect
// ---------------------------------------------------------------------------

TEST(TcpSocketTest, IsNotConnectedOnConstruction) {
  TcpSocket sock;
  EXPECT_FALSE(sock.IsConnected());
}

TEST(TcpSocketTest, ConnectFailsWhenNoServerListening) {
  TcpSocket sock;
  // Port 19999 is unlikely to be in use; if it is, this test will fail —
  // choose a different port.
  auto result = sock.Connect("127.0.0.1", 19999, kConnectTimeout);
  EXPECT_FALSE(result.has_value());
  EXPECT_FALSE(sock.IsConnected());
  // Error should carry the context of the failing call.
  EXPECT_FALSE(result.error().message.empty());
}

TEST(TcpSocketTest, ConnectSucceedsWithMockServer) {
  MockXpsServer server;
  const uint16_t port = server.Start();

  TcpSocket sock;
  auto result = sock.Connect("127.0.0.1", port, kConnectTimeout);
  EXPECT_TRUE(result.has_value()) << result.error().message;
  EXPECT_TRUE(sock.IsConnected());

  server.Stop();
}

TEST(TcpSocketTest, CloseTransitionsToDisconnected) {
  MockXpsServer server;
  const uint16_t port = server.Start();

  TcpSocket sock;
  sock.Connect("127.0.0.1", port, kConnectTimeout);
  ASSERT_TRUE(sock.IsConnected());

  sock.Close();
  EXPECT_FALSE(sock.IsConnected());

  server.Stop();
}

// ---------------------------------------------------------------------------
// Send + ReadUntil
// ---------------------------------------------------------------------------

TEST(TcpSocketTest, SendAndReadUntilRoundTrip) {
  MockXpsServer server;
  server.Expect("Hello\n", "World,EndOfAPI");
  const uint16_t port = server.Start();

  TcpSocket sock;
  ASSERT_TRUE(sock.Connect("127.0.0.1", port, kConnectTimeout).has_value());

  EXPECT_TRUE(sock.Send("Hello\n").has_value());

  auto result = sock.ReadUntil("EndOfAPI", kReadTimeout);
  ASSERT_TRUE(result.has_value()) << result.error().message;
  EXPECT_EQ(*result, "World,EndOfAPI");

  server.Stop();
  server.VerifyAllExpectationsMatched();
}

TEST(TcpSocketTest, MultipleCommandsInSequence) {
  MockXpsServer server;
  server.Expect("CommandA\n", "0,EndOfAPI");
  server.Expect("CommandB\n", "42,SomeValue,EndOfAPI");
  const uint16_t port = server.Start();

  TcpSocket sock;
  ASSERT_TRUE(sock.Connect("127.0.0.1", port, kConnectTimeout).has_value());

  sock.Send("CommandA\n");
  auto r1 = sock.ReadUntil("EndOfAPI", kReadTimeout);
  ASSERT_TRUE(r1.has_value());
  EXPECT_EQ(*r1, "0,EndOfAPI");

  sock.Send("CommandB\n");
  auto r2 = sock.ReadUntil("EndOfAPI", kReadTimeout);
  ASSERT_TRUE(r2.has_value());
  EXPECT_EQ(*r2, "42,SomeValue,EndOfAPI");

  server.Stop();
  server.VerifyAllExpectationsMatched();
}

TEST(TcpSocketTest, ReadUntilReturnsSentinelInclusive) {
  // Verifies that ReadUntil includes the sentinel string in the return value,
  // consistent with how the XPS protocol parser will strip "EndOfAPI".
  MockXpsServer server;
  server.Expect("Ping\n", "Pong,EndOfAPI");
  const uint16_t port = server.Start();

  TcpSocket sock;
  ASSERT_TRUE(sock.Connect("127.0.0.1", port, kConnectTimeout).has_value());

  sock.Send("Ping\n");
  auto result = sock.ReadUntil("EndOfAPI", kReadTimeout);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->ends_with("EndOfAPI"));

  server.Stop();
}

// ---------------------------------------------------------------------------
// Error handling
// ---------------------------------------------------------------------------

TEST(TcpSocketTest, SendFailsWhenNotConnected) {
  TcpSocket sock;
  auto result = sock.Send("anything\n");
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code, -1);
}

TEST(TcpSocketTest, ReadUntilFailsWhenNotConnected) {
  TcpSocket sock;
  auto result = sock.ReadUntil("EndOfAPI", kReadTimeout);
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code, -1);
}

TEST(TcpSocketTest, ReadUntilTimesOutWhenNoData) {
  MockXpsServer server;
  // No expectations — server accepts the connection but never sends anything.
  const uint16_t port = server.Start();

  TcpSocket sock;
  ASSERT_TRUE(sock.Connect("127.0.0.1", port, kConnectTimeout).has_value());

  auto result = sock.ReadUntil("EndOfAPI", std::chrono::milliseconds(150));
  EXPECT_FALSE(result.has_value());
  // Timeout produces a library error (code -1), not a socket error.
  EXPECT_EQ(result.error().code, -1);

  server.Stop();
}
