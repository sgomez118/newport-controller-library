#ifndef NEWPORT_XPS_XPS_GROUP_H_
#define NEWPORT_XPS_XPS_GROUP_H_

#include <string>
#include <vector>

#include "xps_export.h"
#include "xps_positioner.h"

namespace newport::xps {

class NEWPORT_XPS_API XpsGroup {
 public:
  virtual ~XpsGroup() = default;

  const std::string& Name() const;
  const std::vector<XpsPositioner>& Positioners() const;

  // List of Group functions
  virtual int GroupInitialize() = 0;
  virtual int GroupHomeSearch() = 0;

 protected:
  XpsGroup(std::string name, std::vector<XpsPositioner> positioners,
           int max_positioners = 16);

 private:
  std::string name_;
  std::vector<XpsPositioner> positioners_;
};
}  // namespace newport::xps

#endif  // NEWPORT_XPS_XPS_GROUP_H_