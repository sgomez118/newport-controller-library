#ifndef NEWPORT_XPS_XPS_ERROR_H_
#define NEWPORT_XPS_XPS_ERROR_H_

#include <string>

namespace newport::xps {

// Represents any failure returned by the XPS controller or the library itself.
//
//   code    — XPS integer error code.  Negative values from the controller
//             indicate hardware or protocol errors; -1 is used by the library
//             for internal errors (e.g. socket failure, not implemented).
//   command — The raw XPS command string that triggered the error.  Empty for
//             library-side errors that have no corresponding command.
//   message — Human-readable description suitable for logging or display.
//             Includes encoder/configuration errors reported by the controller
//             so diagnostics that were previously hard to trace are captured.
struct XpsError {
  int         code;
  std::string command;
  std::string message;
};

}  // namespace newport::xps

#endif  // NEWPORT_XPS_XPS_ERROR_H_
