#include "newport/xps/xps_group.h"

#include <cassert>

namespace newport::xps {

XpsGroup::XpsGroup(std::string name, std::vector<XpsPositioner> positioners,
                   int max_positioners)
    : name_(std::move(name)), positioners_(std::move(positioners)) {
  assert(static_cast<int>(positioners_.size()) <= max_positioners);
  for (const auto& p : positioners_) {
    assert(p.GroupName() == name_);
  }
}

const std::string& XpsGroup::Name() const { return name_; }

const std::vector<XpsPositioner>& XpsGroup::Positioners() const {
  return positioners_;
}

}  // namespace newport::xps
