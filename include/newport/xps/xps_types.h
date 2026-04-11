#ifndef NEWPORT_XPS_XPS_TYPES_H_
#define NEWPORT_XPS_XPS_TYPES_H_

namespace newport::xps {

// XPS-D group status codes as returned by GroupStatusGet.
//
// Only the states relevant to normal motion workflows are named here.
// The raw integer from GetStatus() can be compared against unlisted codes
// for advanced diagnostics.
enum class GroupStatus : int {
  kNotInitialized              = 0,
  kNotInitializedHardwareError = 1,  // Emergency brake or hardware fault
  kNotReferenced               = 2,
  kConfiguration               = 3,
  kReadyFromDisable            = 7,
  kReady                       = 10,
  kReadyFromMotion             = 11,
  kReadyFromHoming             = 12,
  kDisabled                    = 20,
  kMoving                      = 42,
  kJogging                     = 43,  // Velocity (jog) mode active
  kAnalogTracking              = 44,
  kTrajectory                  = 45,
  kUnknown                     = -1,  // Returned when the code is unrecognized
};

}  // namespace newport::xps

#endif  // NEWPORT_XPS_XPS_TYPES_H_
