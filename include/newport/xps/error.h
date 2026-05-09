#ifndef NEWPORT_XPS_ERROR_H_
#define NEWPORT_XPS_ERROR_H_

#include <expected>
#include <string>

namespace newport::xps {

struct Error {
  int code;
  std::string message;
};

namespace transport_error {
inline constexpr int kSocketCreateFailed = 1001;
inline constexpr int kSocketConnectFailed = 1002;
inline constexpr int kSocketSendFailed = 1003;
inline constexpr int kSocketRecvFailed = 1004;
inline constexpr int kSocketSetoptFailed = 1005;
inline constexpr int kSocketResolveFailed = 1006;
inline constexpr int kSocketClosedByPeer = 1007;
inline constexpr int kSocketTimeout = 1008;
}  // namespace transport_error

template <typename T>
using Result = std::expected<T, Error>;

}  // namespace newport::xps

#endif  // NEWPORT_XPS_ERROR_H_