#ifndef NEWPORT_XPS_XPS_POSITIONER_H_
#define NEWPORT_XPS_XPS_POSITIONER_H_

#include <expected>
#include <functional>
#include <string>

#include "xps_export.h"

namespace newport::xps {

class XpsController;  // forward declaration

struct SGammaProfile {
  double velocity;
  double acceleration;
  double minimum_jerk_time;
  double maximum_jerk_time;
};

struct PositionState {
  double setpoint = 0.0;  // profiler position (where it should be)
  double current = 0.0;   // encoder position after mapping corrections
  double target = 0.0;    // user-commanded final target

  double following_error() const { return setpoint - current; }
};

class NEWPORT_XPS_API XpsPositioner {
 public:
  static std::expected<XpsPositioner, std::string> Create(
      std::string full_name, SGammaProfile sgamma,
      std::function<double(double)> mapping = nullptr);
  const std::string& FullName() const { return full_name_; }
  const std::string& GroupName() const { return group_name_; }
  const SGammaProfile& SGamma() const { return sgamma_; }
  const PositionState& GetPositionState() const { return position_state_; }

 private:
  XpsPositioner(std::string full_name, std::string group_name,
                SGammaProfile sgamma, std::function<double(double)> mapping);
  friend class XpsController;
  void SetPositionState(PositionState state) { position_state_ = state; }
  PositionState position_state_;
  std::string full_name_;   // "Group.Pos1"
  std::string group_name_;  // "Group"
  SGammaProfile sgamma_;
  std::function<double(double)> mapping_;
};
}  // namespace newport::xps

#endif  // NEWPORT_XPS_XPS_POSITIONER_H_