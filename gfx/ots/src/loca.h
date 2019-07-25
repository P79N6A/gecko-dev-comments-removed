



#ifndef OTS_LOCA_H_
#define OTS_LOCA_H_

#include <vector>

#include "ots.h"

namespace ots {

struct OpenTypeLOCA {
  std::vector<uint32_t> offsets;
};

}  

#endif  
