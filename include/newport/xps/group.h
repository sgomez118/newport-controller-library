#ifndef NEWPORT_XPS_GROUP_H_
#define NEWPORT_XPS_GROUP_H_

#include <string>
#include <vector>

#include "positioner.h"
#include "xps_export.h"

namespace newport::xps {

class NEWPORT_XPS_API Group {
 public:
  virtual ~Group() = default;

  const std::string& Name() const;
  const std::vector<Positioner>& Positioners() const;

  // List of Group functions
  virtual int GroupInitialize() = 0;
  virtual int GroupHomeSearch() = 0;

 protected:
  Group(std::string name, std::vector<Positioner> positioners,
        int max_positioners = 16);

 private:
  std::string name_;
  std::vector<Positioner> positioners_;
};
}  // namespace newport::xps

#endif  // NEWPORT_XPS_GROUP_H_