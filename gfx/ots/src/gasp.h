



#ifndef OTS_GASP_H_
#define OTS_GASP_H_

#include <new>
#include <utility>  
#include <vector>

#include "ots.h"

namespace ots {

struct OpenTypeGASP {
  uint16_t version;
  
  std::vector<std::pair<uint16_t, uint16_t> > gasp_ranges;
};

}  

#endif  
