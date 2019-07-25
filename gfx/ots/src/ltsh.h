



#ifndef OTS_LTSH_H_
#define OTS_LTSH_H_

#include <vector>

#include "ots.h"

namespace ots {

struct OpenTypeLTSH {
  uint16_t version;
  std::vector<uint8_t> ypels;
};

}  

#endif  
