



#ifndef OTS_KERN_H_
#define OTS_KERN_H_

#include <vector>

#include "ots.h"

namespace ots {

struct OpenTypeKERNFormat0Pair {
  uint16_t left;
  uint16_t right;
  int16_t value;
};

struct OpenTypeKERNFormat0 {
  uint16_t version;
  uint16_t coverage;
  uint16_t search_range;
  uint16_t entry_selector;
  uint16_t range_shift;
  std::vector<OpenTypeKERNFormat0Pair> pairs;
};





struct OpenTypeKERN {
  uint16_t version;
  std::vector<OpenTypeKERNFormat0> subtables;
};

}  

#endif  
