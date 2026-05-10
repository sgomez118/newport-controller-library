#ifndef NEWPORT_XPS_XPS_SINGLE_AXIS_GROUP_H_
#define NEWPORT_XPS_XPS_SINGLE_AXIS_GROUP_H_

#include "group.h"
#include "positioner.h"
#include "xps_export.h"

namespace newport::xps {

class NEWPORT_XPS_API SingleAxisGroup : public Group {
 public:
  std::expected<SingleAxisGroup, std::string> Create(
      std::string name, std::vector<Positioner> Positioners);
  int Initialize() override;
  int HomeSearch() override;

 private:
  SingleAxisGroup(std::string name, std::vector<Positioner> positioners)
      : Group(std::move(name), std::move(positioners)) {}
};
}  // namespace newport::xps

#endif  // NEWPORT_XPS_XPS_SINGLE_AXIS_GROUP_H_