#include "newport/xps/group.h"

#include <cassert>

#include "newport/xps/positioner.h"

namespace newport::xps {

Group::Group(std::string name, std::vector<Positioner> positioners,
             int max_positioners)
    : name_(std::move(name)), positioners_(std::move(positioners)) {
  assert(static_cast<int>(positioners_.size()) <= max_positioners);
  for (const auto& p : positioners_) {
    assert(p.GroupName() == name_);
  }
}

const std::string& Group::Name() const { return name_; }

const std::vector<Positioner>& Group::Positioners() const {
  return positioners_;
}

}  // namespace newport::xps
