#ifndef NEWPORT_XPS_XPS_POSITIONER_H_
#define NEWPORT_XPS_XPS_POSITIONER_H_

#include <expected>
#include <string>
#include <string_view>

#include "newport/xps/xps_error.h"
#include "newport/xps/xps_export.h"

namespace newport::xps {

class XpsController;

// A handle to a single-axis XPS group, or a named positioner within a
// multi-axis group used for per-axis queries.
//
// Obtained from:
//   XpsController::GetPositioner("STAGE1")       — single-axis group
//   Group::GetPositioner("X")                     — axis within a group
//                                                   (full name: "GROUP.X")
//
// Lightweight value type — cheap to copy.  Two Positioner objects with the
// same name are functionally identical.
//
// Lifetime: the XpsController that produced this Positioner must outlive it.
class NEWPORT_XPS_API Positioner {
 public:
  // Returns the full XPS positioner name (e.g. "STAGE1" or "XYZR.X").
  [[nodiscard]] std::string_view name() const;

  // --- Motion -----------------------------------------------------------

  // Moves to an absolute position (units match the stage configuration:
  // mm for linear, degrees for rotary).
  [[nodiscard]] std::expected<void, XpsError> MoveAbsolute(double position);

  // Moves by a relative displacement from the current setpoint position.
  [[nodiscard]] std::expected<void, XpsError> MoveRelative(double displacement);

  // --- Query ------------------------------------------------------------

  // Returns the current setpoint position.
  [[nodiscard]] std::expected<double, XpsError> GetCurrentPosition();

  // Returns the raw XPS hardware status integer for this positioner.
  // The value is a bitmask — compare against known bit definitions from the
  // XPS-D programmer's manual for detailed diagnostics.
  [[nodiscard]] std::expected<int, XpsError> GetHardwareStatus();

 private:
  friend class XpsController;
  friend class Group;

  Positioner(XpsController* controller, std::string name);

  XpsController* controller_;  // non-owning; must not outlive controller
  std::string    name_;
};

}  // namespace newport::xps

#endif  // NEWPORT_XPS_XPS_POSITIONER_H_
