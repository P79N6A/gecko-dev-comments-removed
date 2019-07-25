



#ifndef OTS_HMTX_H_
#define OTS_HMTX_H_

#include <utility>  
#include <vector>

#include "ots.h"

namespace ots {

struct OpenTypeHMTX {
  std::vector<std::pair<uint16_t, int16_t> > metrics;
  std::vector<int16_t> lsbs;
};

}  

#endif  
