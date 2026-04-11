#ifndef NEWPORT_XPS_XPS_GROUP_H_
#define NEWPORT_XPS_XPS_GROUP_H_

#include <expected>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "newport/xps/xps_error.h"
#include "newport/xps/xps_export.h"
#include "newport/xps/xps_positioner.h"
#include "newport/xps/xps_types.h"

namespace newport::xps {

class XpsController;

// A handle to an XPS group (one or more positioners moving as a coordinated
// unit).  Obtained from XpsController::GetGroup().
//
// Motion methods take and return vectors ordered by positioner index as
// defined in the controller's stage configuration file.  For a single-
// positioner group the vectors will always have exactly one element.
//
// Jog (velocity) mode is a group-level concept in the XPS — individual
// positioners within a group cannot be jogged independently.  Register a
// reconnect callback on XpsController before using jog: a connection drop
// will stop motion on the controller side and the callback is your signal
// to decide whether to restart.
//
// Lightweight value type — cheap to copy.
//
// Lifetime: the XpsController that produced this Group must outlive it.
class NEWPORT_XPS_API Group {
 public:
  // Returns the XPS group name (e.g. "XYZR").
  [[nodiscard]] std::string_view name() const;

  // Returns a Positioner handle for a named positioner within this group.
  // `positioner_name` is the short axis name (e.g. "X"), not the full path.
  // The returned Positioner's full name is "<group>.<positioner_name>".
  [[nodiscard]] Positioner GetPositioner(std::string_view positioner_name);

  // --- Lifecycle --------------------------------------------------------

  // Transitions the group from Not Initialized to Not Referenced state.
  // Must be called before HomeSearch or any motion command.
  [[nodiscard]] std::expected<void, XpsError> Initialize();

  // Runs the homing sequence defined in the stage configuration.
  // On success the group enters Ready state with a valid reference position.
  [[nodiscard]] std::expected<void, XpsError> HomeSearch();

  // Aborts all motion and transitions the group to Not Initialized state.
  // Use when you need to perform a full re-initialize/home cycle.
  [[nodiscard]] std::expected<void, XpsError> Kill();

  // --- Motion -----------------------------------------------------------

  // Moves all positioners to the specified absolute positions.
  // `positions` must have exactly one element per positioner in the group.
  // Units match the stage configuration (mm for linear, degrees for rotary).
  [[nodiscard]] std::expected<void, XpsError> MoveAbsolute(
      std::span<const double> positions);

  // Moves all positioners by the given displacements from current position.
  // `displacements` must have exactly one element per positioner in the group.
  [[nodiscard]] std::expected<void, XpsError> MoveRelative(
      std::span<const double> displacements);

  // Aborts the current motion and brings the group to a controlled stop.
  // The group transitions to Ready state after stopping.
  [[nodiscard]] std::expected<void, XpsError> Stop();

  // --- Jog (velocity mode) ----------------------------------------------

  // Enables jog mode.  The group accelerates to `velocity` (mm/s or deg/s)
  // and moves continuously until JogDisable() is called.
  // `acceleration` is in mm/s² or deg/s².
  [[nodiscard]] std::expected<void, XpsError> JogEnable(double velocity,
                                                         double acceleration);

  // Decelerates the group to a stop and exits jog mode.
  [[nodiscard]] std::expected<void, XpsError> JogDisable();

  // --- Query ------------------------------------------------------------

  // Returns the current setpoint positions for all positioners in the group,
  // in positioner-index order.
  [[nodiscard]] std::expected<std::vector<double>, XpsError>
  GetCurrentPosition();

  // Returns the XPS group status code.
  [[nodiscard]] std::expected<GroupStatus, XpsError> GetStatus();

 private:
  friend class XpsController;

  Group(XpsController* controller, std::string name);

  XpsController* controller_;  // non-owning; must not outlive controller
  std::string    name_;
};

}  // namespace newport::xps

#endif  // NEWPORT_XPS_XPS_GROUP_H_
